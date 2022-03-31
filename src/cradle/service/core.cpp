#include <cradle/service/core.h>

#include <thread>

// Boost.Crc triggers some warnings on MSVC.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4245)
#pragma warning(disable : 4701)
#include <boost/crc.hpp>
#pragma warning(pop)
#else
#include <boost/crc.hpp>
#endif

#include <boost/lexical_cast.hpp>

#include <spdlog/spdlog.h>

#include <cppcoro/schedule_on.hpp>

#include <cradle/encodings/base64.h>
#include <cradle/encodings/lz4.h>
#include <cradle/encodings/native.h>
#include <cradle/fs/file_io.h>
#include <cradle/fs/utilities.h>
#include <cradle/introspection/tasklet.h>
#include <cradle/service/internals.h>

namespace cradle {

void
service_core::reset()
{
    impl_.reset();
}

void
service_core::reset(service_config const& config)
{
    impl_.reset(new detail::service_core_internals{
        .cache = immutable_cache(
            config.immutable_cache ? *config.immutable_cache
                                   : immutable_cache_config(0x40'00'00'00)),
        .http_pool = cppcoro::static_thread_pool(
            config.http_concurrency ? *config.http_concurrency : 36),
        .disk_cache = disk_cache(
            config.disk_cache ? *config.disk_cache
                              : disk_cache_config(none, 0x1'00'00'00'00)),
        .disk_read_pool = cppcoro::static_thread_pool(2),
        .disk_write_pool = thread_pool(2)});
}

service_core::~service_core()
{
}

http_connection_interface&
http_connection_for_thread(service_core& core)
{
    if (core.internals().mock_http)
    {
        thread_local mock_http_connection the_connection(
            *core.internals().mock_http);
        return the_connection;
    }
    else
    {
        static http_request_system the_system;
        thread_local http_connection the_connection(the_system);
        return the_connection;
    }
}

cppcoro::task<http_response>
async_http_request(
    service_core& core, http_request request, tasklet_tracker* client)
{
    std::ostringstream s;
    s << "HTTP: " << request.method << " " << request.url;
    auto tasklet = create_tasklet_tracker("HTTP", s.str(), client);
    co_await core.internals().http_pool.schedule();
    tasklet_run tasklet_run(tasklet);
    null_check_in check_in;
    null_progress_reporter reporter;
    co_return http_connection_for_thread(core).perform_request(
        check_in, reporter, request);
}

cppcoro::task<std::string>
read_file_contents(service_core& core, file_path const& path)
{
    co_await core.internals().disk_read_pool.schedule();
    co_return read_file_contents(path);
}

namespace detail {

string
blob_to_string(blob const& x)
{
    std::ostringstream os;
    os << "<blob - size: " << x.size() + " bytes>";
    return os.str();
}

cppcoro::task<blob>
generic_disk_cached(
    service_core& core,
    id_interface const& id_key,
    std::function<cppcoro::task<blob>()> create_task)
{
    std::string key{boost::lexical_cast<std::string>(id_key)};
    // Check the cache for an existing value.
    auto& cache = core.internals().disk_cache;
    try
    {
        auto entry = cache.find(key);
        if (entry)
        {
            spdlog::get("cradle")->info("disk cache hit on {}", key);

            if (entry->value)
            {
                auto natively_encoded_data = base64_decode(
                    *entry->value, get_mime_base64_character_set());

                blob x{make_blob(std::move(natively_encoded_data))};
                spdlog::get("cradle")->debug(
                    "deserialized: {}", blob_to_string(x));
                co_return x;
            }
            else
            {
                spdlog::get("cradle")->debug("reading file", key);
                auto data = co_await read_file_contents(
                    core, cache.get_path_for_id(entry->id));

                spdlog::get("cradle")->debug("decompressing", key);
                auto original_size
                    = boost::numeric_cast<size_t>(entry->original_size);
                std::unique_ptr<uint8_t[]> decompressed_data(
                    new uint8_t[original_size]);
                lz4::decompress(
                    decompressed_data.get(),
                    original_size,
                    data.data(),
                    data.size());

                spdlog::get("cradle")->debug("checking CRC", key);
                boost::crc_32_type crc;
                crc.process_bytes(decompressed_data.get(), original_size);
                if (crc.checksum() == entry->crc32)
                {
                    spdlog::get("cradle")->debug("decoding", key);
                    blob decoded{
                        reinterpret_pointer_cast<std::byte const>(
                            std::shared_ptr<uint8_t[]>{
                                std::move(decompressed_data)}),
                        original_size};
                    spdlog::get("cradle")->debug("returning", key);
                    co_return decoded;
                }
            }
        }
    }
    catch (...)
    {
        // Something went wrong trying to load the cached value, so just
        // pretend it's not there. (It will be overwritten.)
        spdlog::get("cradle")->warn("error reading disk cache entry {}", key);
    }
    spdlog::get("cradle")->debug("disk cache miss on {}", key);

    // We didn't get it from the cache, so actually create the task to compute
    // the result.
    auto result = co_await create_task();

    // Cache the result.
    core.internals().disk_write_pool.push_task([&core, key, result] {
        auto& cache = core.internals().disk_cache;
        try
        {
            if (result.size() > 1024)
            {
                size_t max_compressed_size
                    = lz4::max_compressed_size(result.size());

                std::unique_ptr<uint8_t[]> compressed_data(
                    new uint8_t[max_compressed_size]);
                size_t actual_compressed_size = lz4::compress(
                    compressed_data.get(),
                    max_compressed_size,
                    result.data(),
                    result.size());

                auto cache_id = cache.initiate_insert(key);
                {
                    auto entry_path = cache.get_path_for_id(cache_id);
                    std::ofstream output;
                    open_file(
                        output,
                        entry_path,
                        std::ios::out | std::ios::trunc | std::ios::binary);
                    output.write(
                        reinterpret_cast<char const*>(compressed_data.get()),
                        actual_compressed_size);
                }
                boost::crc_32_type crc;
                crc.process_bytes(result.data(), result.size());
                cache.finish_insert(cache_id, crc.checksum(), result.size());
            }
            else
            {
                cache.insert(
                    key,
                    base64_encode(
                        reinterpret_cast<uint8_t const*>(result.data()),
                        result.size(),
                        get_mime_base64_character_set()));
            }
        }
        catch (...)
        {
            // Something went wrong trying to write the cached value, so issue
            // a warning and move on.
            spdlog::get("cradle")->warn(
                "error writing disk cache entry {}", key);
        }
    });

    co_return result;
}

} // namespace detail

template<>
cppcoro::task<blob>
disk_cached(
    service_core& core,
    id_interface const& key,
    std::function<cppcoro::task<blob>()> create_task)
{
    return detail::generic_disk_cached(core, key, std::move(create_task));
}

template<>
cppcoro::task<dynamic>
disk_cached(
    service_core& core,
    id_interface const& key,
    std::function<cppcoro::task<dynamic>()> create_task)
{
    auto dynamic_to_blob = [](dynamic x) -> blob {
        return make_blob(write_natively_encoded_value(x));
    };
    auto create_blob_task = [&]() {
        return cppcoro::make_task(
            cppcoro::fmap(dynamic_to_blob, create_task()));
    };
    auto blob_to_dynamic = [](blob x) {
        auto data = reinterpret_cast<uint8_t const*>(x.data());
        return read_natively_encoded_value(data, x.size());
    };
    blob x = co_await detail::generic_disk_cached(core, key, create_blob_task);
    co_return blob_to_dynamic(x);
}

void
init_test_service(service_core& core)
{
    auto cache_dir = file_path("service_disk_cache");

    reset_directory(cache_dir);

    core.reset(service_config(
        immutable_cache_config(0x40'00'00'00),
        disk_cache_config(some(cache_dir.string()), 0x40'00'00'00),
        2,
        2,
        2));
}

mock_http_session&
enable_http_mocking(service_core& core)
{
    core.internals().mock_http = std::make_unique<mock_http_session>();
    return *core.internals().mock_http;
}

} // namespace cradle
