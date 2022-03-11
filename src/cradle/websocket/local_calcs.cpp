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

#include <cradle/core/dynamic.h>
#include <cradle/encodings/msgpack.h>
#include <cradle/encodings/sha256_hash_id.h>
#include <cradle/fs/file_io.h>
#include <cradle/service/core.h>
#include <cradle/thinknode/supervisor.h>
#include <cradle/thinknode/utilities.h>
#include <cradle/utilities/errors.h>
#include <cradle/utilities/functional.h>
#include <cradle/utilities/logging.h>
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
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    string const& account,
    string const& app,
    string const& name,
    std::vector<dynamic> args)
{
    auto const version_info = co_await resolve_context_app(
        service, session, context_id, account, app);
    auto const image = as_private(*version_info.manifest->provider).image;

    co_await get_local_compute_pool_for_image(
        service, std::make_pair(app, image))
        .schedule();
    co_return supervise_thinknode_calculation(
        service, account, app, image, name, std::move(args));
}

} // namespace uncached

cppcoro::task<dynamic>
perform_local_function_calc(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    string const& account,
    string const& app,
    string const& name,
    std::vector<dynamic> args)
{
    auto cache_key = make_sha256_hashed_id(
        "local_function_calc",
        session.api_url,
        context_id,
        account,
        app,
        name,
        map(natively_encoded_sha256, args));

    co_return co_await fully_cached<dynamic>(service, cache_key, [&] {
        return uncached::perform_local_function_calc(
            service, session, context_id, account, app, name, std::move(args));
    });
}

cppcoro::task<dynamic>
coerce_local_calc_result(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    thinknode_type_info const& schema,
    dynamic value)
{
    std::function<cppcoro::task<api_type_info>(
        api_named_type_reference const& ref)>
        look_up_named_type = [&](api_named_type_reference const& ref)
        -> cppcoro::task<api_type_info> {
        co_return co_await resolve_named_type_reference(
            service, session, context_id, ref);
    };
    co_return co_await coerce_value(
        look_up_named_type, as_api_type(schema), std::move(value));
}

cppcoro::task<dynamic>
perform_local_calc(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    std::map<string, dynamic> const& environment,
    thinknode_calc_request request)
{
    auto recursive_call
        = [&](thinknode_calc_request request) -> cppcoro::task<dynamic> {
        return perform_local_calc(
            service, session, context_id, environment, std::move(request));
    };
    auto coercive_call = [&](thinknode_type_info const& schema,
                             dynamic value) -> cppcoro::task<dynamic> {
        return coerce_local_calc_result(
            service, session, context_id, schema, std::move(value));
    };

    switch (get_tag(request))
    {
        case thinknode_calc_request_tag::REFERENCE:
            co_return co_await get_iss_object(
                service, session, context_id, as_reference(request));
        case thinknode_calc_request_tag::VALUE:
            co_return as_value(std::move(request));
        case thinknode_calc_request_tag::FUNCTION: {
            std::vector<dynamic> arg_values;
            auto function = as_function(std::move(request));
            for (auto& arg : function.args)
                arg_values.push_back(co_await recursive_call(std::move(arg)));
            co_return co_await perform_local_function_calc(
                service,
                session,
                context_id,
                function.account,
                function.app,
                function.name,
                std::move(arg_values));
        }
        case thinknode_calc_request_tag::ARRAY: {
            std::vector<dynamic> values;
            auto array = as_array(std::move(request));
            for (auto& item : array.items)
            {
                spdlog::get("cradle")->info(
                    "array.item_schema: {}",
                    boost::lexical_cast<std::string>(array.item_schema));
                values.push_back(co_await coercive_call(
                    array.item_schema,
                    co_await recursive_call(std::move(item))));
            }
            co_return dynamic(values);
        }
        case thinknode_calc_request_tag::ITEM: {
            auto item = as_item(std::move(request));
            co_return co_await coercive_call(
                item.schema,
                cast<dynamic_array>(
                    co_await recursive_call(std::move(item.array)))
                    .at(boost::numeric_cast<size_t>(cast<integer>(
                        co_await recursive_call(std::move(item.index))))));
        }
        case thinknode_calc_request_tag::OBJECT: {
            auto object = as_object(std::move(request));
            dynamic_map result;
            for (auto& property : object.properties)
            {
                result[dynamic(property.first)]
                    = co_await recursive_call(property.second);
            }
            co_return co_await coercive_call(object.schema, std::move(result));
        }
        case thinknode_calc_request_tag::PROPERTY: {
            auto property = as_property(std::move(request));
            co_return co_await coercive_call(
                property.schema,
                cast<dynamic_map>(
                    co_await recursive_call(std::move(property.object)))
                    .at(cast<string>(
                        co_await recursive_call(std::move(property.field)))));
        }
        case thinknode_calc_request_tag::LET: {
            auto let = as_let(std::move(request));
            std::map<string, dynamic> extended_environment = environment;
            for (auto& v : let.variables)
            {
                extended_environment[v.first]
                    = co_await recursive_call(std::move(v.second));
            }
            co_return co_await perform_local_calc(
                service,
                session,
                context_id,
                extended_environment,
                std::move(let.in));
        }
        case thinknode_calc_request_tag::VARIABLE:
            co_return environment.at(as_variable(request));
        case thinknode_calc_request_tag::META: {
            auto meta = as_meta(std::move(request));
            auto generated_request = from_dynamic<thinknode_calc_request>(
                co_await recursive_call(std::move(meta.generator)));
            co_return co_await coercive_call(
                meta.schema, co_await recursive_call(generated_request));
        }
        case thinknode_calc_request_tag::CAST: {
            auto cast = as_cast(std::move(request));
            co_return co_await coercive_call(
                cast.schema, co_await recursive_call(std::move(cast.object)));
        }
        default:
            CRADLE_THROW(
                invalid_enum_value()
                << enum_id_info("thinknode_calc_request_tag")
                << enum_value_info(static_cast<int>(get_tag(request))));
    }
}

cppcoro::task<dynamic>
perform_local_calc(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    thinknode_calc_request request)
{
    co_return co_await perform_local_calc(
        service, session, context_id, {}, std::move(request));
}

cppcoro::task<dynamic>
resolve_calc_to_value(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    thinknode_calc_request request);

cppcoro::task<std::string>
resolve_calc_to_iss_object(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    thinknode_calc_request request);

} // namespace cradle
