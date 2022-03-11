#include <cradle/websocket/hybrid_calcs.h>

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

#include <cppcoro/when_all.hpp>

#include <cradle/core/dynamic.h>
#include <cradle/encodings/msgpack.h>
#include <cradle/encodings/sha256_hash_id.h>
#include <cradle/fs/file_io.h>
#include <cradle/service/core.h>
#include <cradle/thinknode/calc.h>
#include <cradle/thinknode/iss.h>
#include <cradle/thinknode/supervisor.h>
#include <cradle/thinknode/utilities.h>
#include <cradle/utilities/errors.h>
#include <cradle/utilities/functional.h>
#include <cradle/utilities/logging.h>

namespace cradle {

// signatures for functions that we're temporarily borrowing from other places:

cppcoro::shared_task<dynamic>
get_iss_object(
    service_core& service,
    thinknode_session session,
    string context_id,
    string object_id,
    bool ignore_upgrades = false);

cppcoro::shared_task<api_type_info>
resolve_named_type_reference(
    service_core& service,
    thinknode_session session,
    string context_id,
    api_named_type_reference ref);

cppcoro::task<thinknode_app_version_info>
resolve_context_app(
    service_core& service,
    thinknode_session session,
    string context_id,
    string account,
    string app);

cppcoro::task<dynamic>
perform_local_function_calc(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    string const& account,
    string const& app,
    string const& name,
    std::vector<dynamic> args);

cppcoro::task<dynamic>
coerce_local_calc_result(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    thinknode_type_info const& schema,
    dynamic value);

// end of temporary borrowing

cppcoro::task<std::string>
resolve_hybrid_calc_to_iss_object(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    std::map<string, hybrid_calculation_request> const& environment,
    hybrid_calculation_request request);

cppcoro::task<dynamic>
resolve_hybrid_calc_to_value(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    std::map<string, hybrid_calculation_request> const& environment,
    hybrid_calculation_request request)
{
    auto recursive_call
        = [&](hybrid_calculation_request request) -> cppcoro::task<dynamic> {
        return resolve_hybrid_calc_to_value(
            service, session, context_id, environment, std::move(request));
    };
    auto coercive_call = [&](thinknode_type_info const& schema,
                             dynamic value) -> cppcoro::task<dynamic> {
        return coerce_local_calc_result(
            service, session, context_id, schema, std::move(value));
    };

    switch (get_tag(request))
    {
        case hybrid_calculation_request_tag::REFERENCE:
            co_return co_await get_iss_object(
                service, session, context_id, as_reference(request));
        case hybrid_calculation_request_tag::VALUE:
            co_return as_value(std::move(request));
        case hybrid_calculation_request_tag::LAMBDA: {
            std::vector<dynamic> arg_values;
            auto lambda = as_lambda(std::move(request));
            // TODO: Do this in parallel.
            for (auto& arg : lambda.args)
                arg_values.push_back(co_await recursive_call(std::move(arg)));
            co_return lambda.function.object(std::move(arg_values));
        }
        case hybrid_calculation_request_tag::FUNCTION:
            // If the function is specifically requested to be executed
            // remotely in Thinknode, then resolve the calculation to an ISS
            // object and download the associated data.
            if (as_function(request).host
                == execution_host_selection::THINKNODE)
            {
                co_return co_await get_iss_object(
                    service,
                    session,
                    context_id,
                    co_await resolve_hybrid_calc_to_iss_object(
                        service,
                        session,
                        context_id,
                        environment,
                        std::move(request)));
            }
            // Otherwise, we evaluate the function locally.
            else
            {
                std::vector<dynamic> arg_values;
                auto function = as_function(std::move(request));
                // TODO: Do this in parallel.
                for (auto& arg : function.args)
                {
                    arg_values.push_back(
                        co_await recursive_call(std::move(arg)));
                }
                co_return co_await perform_local_function_calc(
                    service,
                    session,
                    context_id,
                    function.account,
                    function.app,
                    function.name,
                    std::move(arg_values));
            }
        case hybrid_calculation_request_tag::ARRAY: {
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
        case hybrid_calculation_request_tag::ITEM: {
            auto item = as_item(std::move(request));
            co_return co_await coercive_call(
                item.schema,
                cast<dynamic_array>(
                    co_await recursive_call(std::move(item.array)))
                    .at(boost::numeric_cast<size_t>(cast<integer>(
                        co_await recursive_call(std::move(item.index))))));
        }
        case hybrid_calculation_request_tag::OBJECT: {
            auto object = as_object(std::move(request));
            dynamic_map result;
            for (auto& property : object.properties)
            {
                result[dynamic(property.first)]
                    = co_await recursive_call(property.second);
            }
            co_return co_await coercive_call(object.schema, std::move(result));
        }
        case hybrid_calculation_request_tag::PROPERTY: {
            auto property = as_property(std::move(request));
            co_return co_await coercive_call(
                property.schema,
                cast<dynamic_map>(
                    co_await recursive_call(std::move(property.object)))
                    .at(cast<string>(
                        co_await recursive_call(std::move(property.field)))));
        }
        case hybrid_calculation_request_tag::LET: {
            auto let = as_let(std::move(request));
            std::map<string, hybrid_calculation_request> extended_environment
                = environment;
            for (auto& v : let.variables)
                extended_environment[v.first] = std::move(v.second);
            co_return co_await resolve_hybrid_calc_to_value(
                service,
                session,
                context_id,
                extended_environment,
                std::move(let.in));
        }
        case hybrid_calculation_request_tag::VARIABLE:
            co_return co_await recursive_call(
                environment.at(as_variable(request)));
        case hybrid_calculation_request_tag::META: {
            auto meta = as_meta(std::move(request));
            auto generated_request = from_dynamic<hybrid_calculation_request>(
                co_await recursive_call(std::move(meta.generator)));
            co_return co_await coercive_call(
                meta.schema, co_await recursive_call(generated_request));
        }
        case hybrid_calculation_request_tag::CAST: {
            auto cast = as_cast(std::move(request));
            co_return co_await coercive_call(
                cast.schema, co_await recursive_call(std::move(cast.object)));
        }
        default:
            CRADLE_THROW(
                invalid_enum_value()
                << enum_id_info("calculation_request_tag")
                << enum_value_info(static_cast<int>(get_tag(request))));
    }
}

cppcoro::task<std::string>
resolve_hybrid_calc_to_iss_object(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    std::map<string, hybrid_calculation_request> const& environment,
    hybrid_calculation_request request)
{
    // For most calculation types, the resolution will be to convert the hybrid
    // to a regular Thinknode calculation in "shallow form" (where all
    // subcalculations are stored as references) and then post that
    // calculation.
    auto recurse = [&](hybrid_calculation_request calc)
        -> cppcoro::task<calculation_request> {
        co_return make_calculation_request_with_reference(
            co_await resolve_hybrid_calc_to_iss_object(
                service, session, context_id, environment, std::move(calc)));
    };
    auto post_calc
        = [&](calculation_request calc) -> cppcoro::shared_task<std::string> {
        return post_calculation(service, session, context_id, std::move(calc));
    };

    // For other types, the resolution will be to simply post the result as a
    // value to Thinknode via this helper.
    auto post_value = [&](dynamic value) -> cppcoro::shared_task<std::string> {
        return post_iss_object(
            service,
            session,
            context_id,
            make_thinknode_type_info_with_dynamic_type(
                thinknode_dynamic_type()),
            std::move(value));
    };

    switch (get_tag(request))
    {
        case hybrid_calculation_request_tag::REFERENCE:
            co_return as_reference(std::move(request));
        case hybrid_calculation_request_tag::VALUE:
            co_return co_await post_value(as_value(std::move(request)));
        case hybrid_calculation_request_tag::LAMBDA: {
            co_return co_await post_value(
                co_await resolve_hybrid_calc_to_value(
                    service,
                    session,
                    context_id,
                    environment,
                    std::move(request)));
        }
        case hybrid_calculation_request_tag::FUNCTION: {
            // If the function is specifically requested to be executed
            // locally, then resolve it locally to a value and upload that to
            // Thinknode.
            if (as_function(request).host == execution_host_selection::LOCAL)
            {
                co_return co_await post_value(
                    co_await resolve_hybrid_calc_to_value(
                        service,
                        session,
                        context_id,
                        environment,
                        std::move(request)));
            }
            // Otherwise, we request Thinknode to evaluate it.
            else
            {
                auto subtasks
                    = map(recurse, std::move(as_function(request).args));
                co_return co_await post_calc(
                    make_calculation_request_with_function(
                        make_function_application(
                            as_function(request).account,
                            as_function(request).app,
                            as_function(request).name,
                            as_function(request).level,
                            co_await cppcoro::when_all(std::move(subtasks)))));
            }
        }
        case hybrid_calculation_request_tag::ARRAY: {
            auto subtasks = map(recurse, std::move(as_array(request).items));
            co_return co_await post_calc(make_calculation_request_with_array(
                make_calculation_array_request(
                    co_await cppcoro::when_all(std::move(subtasks)),
                    as_array(request).item_schema)));
        }
        case hybrid_calculation_request_tag::ITEM:
            co_return co_await post_calc(make_calculation_request_with_item(
                make_calculation_item_request(
                    co_await recurse(as_item(request).array),
                    co_await recurse(as_item(request).index),
                    as_item(request).schema)));
        case hybrid_calculation_request_tag::OBJECT: {
            std::map<string, calculation_request> properties;
            for (auto& property : as_object(request).properties)
            {
                properties[property.first]
                    = co_await recurse(std::move(property.second));
            }
            co_return co_await post_calc(make_calculation_request_with_object(
                make_calculation_object_request(
                    properties, as_object(request).schema)));
        }
        case hybrid_calculation_request_tag::PROPERTY:
            co_return co_await post_calc(
                make_calculation_request_with_property(
                    make_calculation_property_request(
                        co_await recurse(as_property(request).object),
                        co_await recurse(as_property(request).field),
                        as_property(request).schema)));
        case hybrid_calculation_request_tag::LET: {
            auto let = as_let(std::move(request));
            std::map<string, hybrid_calculation_request> extended_environment
                = environment;
            for (auto& v : let.variables)
                extended_environment[v.first] = std::move(v.second);
            co_return co_await resolve_hybrid_calc_to_iss_object(
                service,
                session,
                context_id,
                extended_environment,
                std::move(let.in));
        }
        case hybrid_calculation_request_tag::VARIABLE:
            co_return co_await resolve_hybrid_calc_to_iss_object(
                service,
                session,
                context_id,
                environment,
                environment.at(as_variable(request)));
        case hybrid_calculation_request_tag::META:
            co_return co_await post_calc(make_calculation_request_with_meta(
                make_meta_calculation_request(
                    co_await recurse(as_meta(request).generator),
                    as_meta(request).schema)));
        case hybrid_calculation_request_tag::CAST:
            co_return co_await post_calc(make_calculation_request_with_cast(
                make_calculation_cast_request(
                    as_cast(request).schema,
                    co_await recurse(as_cast(request).object))));
        default:
            CRADLE_THROW(
                invalid_enum_value()
                << enum_id_info("calculation_request_tag")
                << enum_value_info(static_cast<int>(get_tag(request))));
    }
}

cppcoro::task<dynamic>
resolve_hybrid_calc_to_value(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    hybrid_calculation_request request)
{
    co_return co_await resolve_hybrid_calc_to_value(
        service, session, context_id, {}, std::move(request));
}

cppcoro::task<std::string>
resolve_hybrid_calc_to_iss_object(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    hybrid_calculation_request request)
{
    co_return co_await resolve_hybrid_calc_to_iss_object(
        service, session, context_id, {}, std::move(request));
}

} // namespace cradle
