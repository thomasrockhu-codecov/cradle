#include <cradle/websocket/hybrid_calcs.h>

#include <thread>

#include <cppcoro/sync_wait.hpp>

#include <cradle/utilities/testing.h>

#include <cradle/encodings/base64.h>
#include <cradle/utilities/environment.h>
#include <cradle/websocket/messages.hpp>

using namespace cradle;

namespace {

dynamic
dynamic_subtract(dynamic_array args)
{
    return cast<double>(args.at(0)) - cast<double>(args.at(1));
}

} // namespace

TEST_CASE("local hybrid calcs", "[hybrid_calcs][ws]")
{
    service_core core;
    init_test_service(core);

    thinknode_session session;
    session.api_url = "https://mgh.thinknode.io/api/v1.0";
    session.access_token
        = get_environment_variable("CRADLE_THINKNODE_API_TOKEN");

    auto eval = [&](hybrid_calculation_request const& request) {
        return cppcoro::sync_wait(resolve_hybrid_calc_to_value(
            core, session, "5dadeb4a004073e81b5e096255e83652", request));
    };

    // value
    REQUIRE(
        eval(make_hybrid_calculation_request_with_value(dynamic{2.5}))
        == dynamic{2.5});
    REQUIRE(
        eval(make_hybrid_calculation_request_with_value(dynamic{"foobar"}))
        == dynamic{"foobar"});
    REQUIRE(
        eval(make_hybrid_calculation_request_with_value(
            dynamic({1.0, true, "x"})))
        == dynamic({1.0, true, "x"}));

    // reference
    REQUIRE(
        eval(make_hybrid_calculation_request_with_reference(
            "5abd360900c0b14726b4ba1e6e5cdc12"))
        == dynamic(
            {{"demographics",
              {
                  {"birthdate", {{"some", "1800-01-01"}}},
                  {"sex", {{"some", "o"}}},
              }},
             {"medical_record_number", "017-08-01"},
             {"name",
              {{"family_name", "Astroid"},
               {"given_name", "v2"},
               {"middle_name", ""},
               {"prefix", ""},
               {"suffix", ""}}}}));

    // function
    REQUIRE(
        eval(make_hybrid_calculation_request_with_function(
            make_hybrid_function_application(
                "mgh",
                "dosimetry",
                "addition",
                execution_host_selection::LOCAL,
                none,
                {make_hybrid_calculation_request_with_value(dynamic(2.0)),
                 make_hybrid_calculation_request_with_value(dynamic(0.125))})))
        == dynamic(2.125));

    // lambda w/ actual lambda
    REQUIRE(
        eval(make_hybrid_calculation_request_with_lambda(
            make_lambda_calculation(
                make_function([](dynamic_array args) {
                    return cast<double>(args.at(0)) - cast<double>(args.at(1));
                }),
                {make_hybrid_calculation_request_with_value(dynamic(7.0)),
                 make_hybrid_calculation_request_with_value(dynamic(1.0))})))
        == dynamic(6.0));

    // lambda w/ function pointer
    REQUIRE(
        eval(make_hybrid_calculation_request_with_lambda(
            make_lambda_calculation(
                make_function(dynamic_subtract),
                {make_hybrid_calculation_request_with_value(dynamic(8.0)),
                 make_hybrid_calculation_request_with_value(dynamic(1.0))})))
        == dynamic(7.0));

    // array
    REQUIRE(
        eval(make_hybrid_calculation_request_with_array(
            make_hybrid_array_request(
                {make_hybrid_calculation_request_with_value(
                     dynamic(integer(2))),
                 make_hybrid_calculation_request_with_value(
                     dynamic(integer(0))),
                 make_hybrid_calculation_request_with_value(
                     dynamic(integer(3)))},
                make_thinknode_type_info_with_integer_type(
                    make_thinknode_integer_type()))))
        == dynamic({integer(2), integer(0), integer(3)}));

    // item
    REQUIRE(
        eval(
            make_hybrid_calculation_request_with_item(make_hybrid_item_request(
                make_hybrid_calculation_request_with_value(
                    dynamic({integer(2), integer(0), integer(3)})),
                make_hybrid_calculation_request_with_value(
                    dynamic(integer(1))),
                make_thinknode_type_info_with_integer_type(
                    make_thinknode_integer_type()))))
        == dynamic(integer(0)));

    // object
    REQUIRE(
        eval(make_hybrid_calculation_request_with_object(
            make_hybrid_object_request(
                {{"two",
                  make_hybrid_calculation_request_with_value(
                      dynamic(integer(2)))},
                 {"oh",
                  make_hybrid_calculation_request_with_value(
                      dynamic(integer(0)))},
                 {"three",
                  make_hybrid_calculation_request_with_value(
                      dynamic(integer(3)))}},
                make_thinknode_type_info_with_structure_type(
                    make_thinknode_structure_info(
                        {{"two",
                          make_thinknode_structure_field_info(
                              "the two",
                              some(false),
                              make_thinknode_type_info_with_integer_type(
                                  make_thinknode_integer_type()))},
                         {"oh",
                          make_thinknode_structure_field_info(
                              "the oh",
                              some(false),
                              make_thinknode_type_info_with_integer_type(
                                  make_thinknode_integer_type()))},
                         {"three",
                          make_thinknode_structure_field_info(
                              "the three",
                              some(false),
                              make_thinknode_type_info_with_integer_type(
                                  make_thinknode_integer_type()))}})))))
        == dynamic(
            {{"two", integer(2)}, {"oh", integer(0)}, {"three", integer(3)}}));

    // property
    REQUIRE(
        eval(make_hybrid_calculation_request_with_property(
            make_hybrid_property_request(
                make_hybrid_calculation_request_with_value(dynamic(
                    {{"two", integer(2)},
                     {"oh", integer(0)},
                     {"three", integer(3)}})),
                make_hybrid_calculation_request_with_value(dynamic("oh")),
                make_thinknode_type_info_with_integer_type(
                    make_thinknode_integer_type()))))
        == dynamic(integer(0)));

    // let/variable
    REQUIRE(
        eval(make_hybrid_calculation_request_with_let(make_hybrid_let_request(
            {{"x",
              make_hybrid_calculation_request_with_value(
                  dynamic(integer(2)))}},
            make_hybrid_calculation_request_with_variable("x"))))
        == dynamic(integer(2)));

    // meta
    REQUIRE(
        eval(
            make_hybrid_calculation_request_with_meta(make_hybrid_meta_request(
                make_hybrid_calculation_request_with_value(
                    dynamic({{"value", integer(1)}})),
                make_thinknode_type_info_with_integer_type(
                    make_thinknode_integer_type()))))
        == dynamic(integer(1)));

    // cast
    REQUIRE(
        eval(
            make_hybrid_calculation_request_with_cast(make_hybrid_cast_request(
                make_thinknode_type_info_with_integer_type(
                    make_thinknode_integer_type()),
                make_hybrid_calculation_request_with_value(dynamic(0.0)))))
        == dynamic(integer(0)));
}

TEST_CASE("mixed hybrid calcs", "[hybrid_calcs][ws]")
{
    service_core core;
    init_test_service(core);

    thinknode_session session;
    session.api_url = "https://mgh.thinknode.io/api/v1.0";
    session.access_token
        = get_environment_variable("CRADLE_THINKNODE_API_TOKEN");

    auto eval = [&](hybrid_calculation_request const& request) {
        return cppcoro::sync_wait(resolve_hybrid_calc_to_value(
            core, session, "5dadeb4a004073e81b5e096255e83652", request));
    };

    auto local_calc = make_hybrid_calculation_request_with_function(
        make_hybrid_function_application(
            "mgh",
            "dosimetry",
            "addition",
            execution_host_selection::LOCAL,
            none,
            {make_hybrid_calculation_request_with_value(dynamic(1.0)),
             make_hybrid_calculation_request_with_value(dynamic(2.0))}));

    auto remote_calc = make_hybrid_calculation_request_with_function(
        make_hybrid_function_application(
            "mgh",
            "dosimetry",
            "addition",
            execution_host_selection::THINKNODE,
            none,
            {make_hybrid_calculation_request_with_value(dynamic(3.0)),
             local_calc}));

    auto lambda_calc
        = make_hybrid_calculation_request_with_lambda(make_lambda_calculation(
            make_function([](dynamic_array args) {
                return cast<double>(args.at(0)) - cast<double>(args.at(1));
            }),
            {make_hybrid_calculation_request_with_value(dynamic(7.0)),
             remote_calc}));

    auto function_pointer_calc
        = make_hybrid_calculation_request_with_lambda(make_lambda_calculation(
            make_function(dynamic_subtract),
            {make_hybrid_calculation_request_with_value(dynamic(8.0)),
             lambda_calc}));

    REQUIRE(eval(function_pointer_calc) == dynamic(7.0));
}
