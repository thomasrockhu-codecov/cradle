#include <cradle/websocket/local_calcs.h>

#include <picosha2.h>

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

#include <cradle/inner/fs/file_io.h>
#include <cradle/inner/utilities/errors.h>
#include <cradle/inner/utilities/functional.h>
#include <cradle/thinknode/supervisor.h>
#include <cradle/thinknode/utilities.h>
#include <cradle/typing/core/dynamic.h>
#include <cradle/typing/encodings/msgpack.h>
#include <cradle/typing/encodings/sha256_hash_id.h>
#include <cradle/typing/service/core.h>
#include <cradle/typing/utilities/logging.h>
#include <cradle/websocket/server_api.h>

namespace cradle {

cppcoro::static_thread_pool&
get_local_compute_pool_for_image(
    service_core& service,
    std::pair<std::string, thinknode_provider_image_info> const& tag)
{
    return service.internals()
        .local_compute_pool.try_emplace(tag, 4)
        .first->second;
}

namespace uncached {

cppcoro::task<dynamic>
perform_local_function_calc(
    thinknode_request_context ctx,
    string const& context_id,
    string const& account,
    string const& app,
    string const& name,
    std::vector<dynamic> args)
{
    auto const version_info
        = co_await resolve_context_app(ctx, context_id, account, app);
    auto const image = as_private(*version_info.manifest->provider).image;

    auto pool_name = std::string{"local@"} + app;
    auto tasklet
        = create_tasklet_tracker(pool_name, "local calc", ctx.tasklet);
    co_await get_local_compute_pool_for_image(
        ctx.service, std::make_pair(app, image))
        .schedule();

    auto run_guard = tasklet_run(tasklet);
    co_return supervise_thinknode_calculation(
        ctx.service, account, app, image, name, std::move(args));
}

} // namespace uncached

cppcoro::task<dynamic>
perform_local_function_calc(
    thinknode_request_context ctx,
    string context_id,
    string account,
    string app,
    string name,
    std::vector<dynamic> args)
{
    auto cache_key = make_captured_sha256_hashed_id(
        "local_function_calc",
        ctx.session.api_url,
        context_id,
        account,
        app,
        name,
        map(CRADLE_LAMBDIFY(natively_encoded_sha256), args));

    tasklet_await around_await(
        ctx.tasklet, "perform_local_function_calc", *cache_key);
    auto task_creator = [&] {
        return uncached::perform_local_function_calc(
            ctx, context_id, account, app, name, std::move(args));
    };
    auto result
        = co_await fully_cached<dynamic>(ctx.service, cache_key, task_creator);
    co_return result;
}

cppcoro::task<dynamic>
coerce_local_calc_result(
    thinknode_request_context ctx,
    string const& context_id,
    thinknode_type_info const& schema,
    dynamic value)
{
    std::function<cppcoro::task<api_type_info>(
        api_named_type_reference const& ref)>
        look_up_named_type = [&](api_named_type_reference const& ref)
        -> cppcoro::task<api_type_info> {
        co_return co_await resolve_named_type_reference(ctx, context_id, ref);
    };
    co_return co_await coerce_value(
        look_up_named_type, as_api_type(schema), std::move(value));
}

} // namespace cradle
