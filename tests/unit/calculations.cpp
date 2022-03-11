#include <cradle/websocket/calculations.h>

#include <thread>

#include <cppcoro/sync_wait.hpp>

#include <cradle/utilities/testing.h>

#include <cradle/encodings/base64.h>
#include <cradle/utilities/environment.h>
#include <cradle/websocket/messages.hpp>

using namespace cradle;

namespace {

// Test all the relevant ID operations on a pair of equal IDs.
static void
test_equal_ids(id_interface const& a, id_interface const& b)
{
    REQUIRE(a == b);
    REQUIRE(b == a);
    REQUIRE(!(a < b));
    REQUIRE(!(b < a));
    REQUIRE(a.hash() == b.hash());
    auto a_string = boost::lexical_cast<std::string>(a);
    auto b_string = boost::lexical_cast<std::string>(b);
    REQUIRE(a_string == b_string);
}

// Test all the ID operations on a pair of different IDs.
void
test_different_ids(
    id_interface const& a, id_interface const& b, bool ignore_strings = false)
{
    REQUIRE(a != b);
    REQUIRE((a < b && !(b < a) || b < a && !(a < b)));
    REQUIRE(a.hash() != b.hash());
    auto a_string = boost::lexical_cast<std::string>(a);
    auto b_string = boost::lexical_cast<std::string>(b);
    // Raw function pointers are implicitly converted to bool
    if (!ignore_strings)
    {
        REQUIRE(a_string != b_string);
    }
}

int
add(int x, int y)
{
    return x + y;
}

int
subtract(int x, int y)
{
    return x - y;
}

} // namespace

TEST_CASE("function IDs", "[calcs][ws]")
{
    auto lambda_add = [](int x, int y) { return x + y; };
    auto lambda_subtract = [](int x, int y) { return x - y; };

    test_equal_ids(make_function_id(add), make_function_id(add));
    test_equal_ids(make_function_id(subtract), make_function_id(subtract));
    test_different_ids(
        make_function_id(add), make_function_id(subtract), true);

    test_equal_ids(make_function_id(lambda_add), make_function_id(lambda_add));
    test_equal_ids(
        make_function_id(lambda_subtract), make_function_id(lambda_subtract));
    test_different_ids(
        make_function_id(lambda_add), make_function_id(lambda_subtract));

    test_different_ids(make_function_id(add), make_function_id(lambda_add));
    test_different_ids(
        make_function_id(subtract), make_function_id(lambda_subtract));

    captured_id captured(make_function_id(add));
    REQUIRE(captured.matches(make_function_id(add)));
    REQUIRE(!captured.matches(make_function_id(subtract)));
    REQUIRE(!captured.matches(make_function_id(lambda_add)));

    captured.capture(make_function_id(lambda_add));
    REQUIRE(captured.matches(make_function_id(lambda_add)));
    REQUIRE(!captured.matches(make_function_id(lambda_subtract)));
    REQUIRE(!captured.matches(make_function_id(add)));
}

namespace {

dynamic
dynamic_subtract(dynamic_array args)
{
    return cast<double>(args.at(0)) - cast<double>(args.at(1));
}

} // namespace

TEST_CASE("individual calcs", "[calcs][ws]")
{
    service_core core;
    init_test_service(core);

    thinknode_session session;
    session.api_url = "https://mgh.thinknode.io/api/v1.0";
    session.access_token
        = get_environment_variable("CRADLE_THINKNODE_API_TOKEN");

    auto eval = [&](calculation_request const& request) {
        return cppcoro::sync_wait(resolve_calc_to_value(
            core, session, "5dadeb4a004073e81b5e096255e83652", request));
    };

    // value
    REQUIRE(
        eval(make_calculation_request_with_value(dynamic{2.5}))
        == dynamic{2.5});
    REQUIRE(
        eval(make_calculation_request_with_value(dynamic{"foobar"}))
        == dynamic{"foobar"});
    REQUIRE(
        eval(make_calculation_request_with_value(dynamic({1.0, true, "x"})))
        == dynamic({1.0, true, "x"}));

    // reference
    REQUIRE(
        eval(make_calculation_request_with_reference(
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

#ifdef LOCAL_DOCKER_TESTING
    // function
    REQUIRE(
        eval(make_calculation_request_with_function(make_function_application(
            "mgh",
            "dosimetry",
            "addition",
            execution_host_selection::LOCAL,
            none,
            {make_calculation_request_with_value(dynamic(2.0)),
             make_calculation_request_with_value(dynamic(0.125))})))
        == dynamic(2.125));
#endif

    // lambda w/ actual lambda
    REQUIRE(
        eval(make_calculation_request_with_lambda(make_lambda_calculation(
            make_function([](dynamic_array args) {
                return cast<double>(args.at(0)) - cast<double>(args.at(1));
            }),
            {make_calculation_request_with_value(dynamic(7.0)),
             make_calculation_request_with_value(dynamic(1.0))})))
        == dynamic(6.0));

    // lambda w/ function pointer
    REQUIRE(
        eval(make_calculation_request_with_lambda(make_lambda_calculation(
            make_function(dynamic_subtract),
            {make_calculation_request_with_value(dynamic(8.0)),
             make_calculation_request_with_value(dynamic(1.0))})))
        == dynamic(7.0));

    // array
    REQUIRE(
        eval(make_calculation_request_with_array(make_array_calc_request(
            {make_calculation_request_with_value(dynamic(integer(2))),
             make_calculation_request_with_value(dynamic(integer(0))),
             make_calculation_request_with_value(dynamic(integer(3)))},
            make_thinknode_type_info_with_integer_type(
                make_thinknode_integer_type()))))
        == dynamic({integer(2), integer(0), integer(3)}));

    // item
    REQUIRE(
        eval(make_calculation_request_with_item(make_item_calc_request(
            make_calculation_request_with_value(
                dynamic({integer(2), integer(0), integer(3)})),
            make_calculation_request_with_value(dynamic(integer(1))),
            make_thinknode_type_info_with_integer_type(
                make_thinknode_integer_type()))))
        == dynamic(integer(0)));

    // object
    REQUIRE(
        eval(make_calculation_request_with_object(make_object_calc_request(
            {{"two", make_calculation_request_with_value(dynamic(integer(2)))},
             {"oh", make_calculation_request_with_value(dynamic(integer(0)))},
             {"three",
              make_calculation_request_with_value(dynamic(integer(3)))}},
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
        eval(make_calculation_request_with_property(make_property_calc_request(
            make_calculation_request_with_value(dynamic(
                {{"two", integer(2)},
                 {"oh", integer(0)},
                 {"three", integer(3)}})),
            make_calculation_request_with_value(dynamic("oh")),
            make_thinknode_type_info_with_integer_type(
                make_thinknode_integer_type()))))
        == dynamic(integer(0)));

    // let/variable
    REQUIRE(
        eval(make_calculation_request_with_let(make_let_calc_request(
            {{"x", make_calculation_request_with_value(dynamic(integer(2)))}},
            make_calculation_request_with_variable("x"))))
        == dynamic(integer(2)));

    // meta
    REQUIRE(
        eval(make_calculation_request_with_meta(make_meta_calc_request(
            make_calculation_request_with_value(
                dynamic({{"value", integer(1)}})),
            make_thinknode_type_info_with_integer_type(
                make_thinknode_integer_type()))))
        == dynamic(integer(1)));

    // cast
    REQUIRE(
        eval(make_calculation_request_with_cast(make_cast_calc_request(
            make_thinknode_type_info_with_integer_type(
                make_thinknode_integer_type()),
            make_calculation_request_with_value(dynamic(0.0)))))
        == dynamic(integer(0)));
}

TEST_CASE("mixed calcs", "[calcs][ws]")
{
    service_core core;
    init_test_service(core);

    thinknode_session session;
    session.api_url = "https://mgh.thinknode.io/api/v1.0";
    session.access_token
        = get_environment_variable("CRADLE_THINKNODE_API_TOKEN");

    auto eval = [&](calculation_request const& request) {
        return cppcoro::sync_wait(resolve_calc_to_value(
            core, session, "5dadeb4a004073e81b5e096255e83652", request));
    };

#ifdef LOCAL_DOCKER_TESTING
    auto local_calc
        = make_calculation_request_with_function(make_function_application(
            "mgh",
            "dosimetry",
            "addition",
            execution_host_selection::LOCAL,
            none,
            {make_calculation_request_with_value(dynamic(1.0)),
             make_calculation_request_with_value(dynamic(2.0))}));
#else
    auto local_calc = make_calculation_request_with_value(dynamic(3.0));
#endif

    auto remote_calc
        = make_calculation_request_with_function(make_function_application(
            "mgh",
            "dosimetry",
            "addition",
            execution_host_selection::THINKNODE,
            none,
            {make_calculation_request_with_value(dynamic(3.0)), local_calc}));

    auto lambda_calc
        = make_calculation_request_with_lambda(make_lambda_calculation(
            make_function([](dynamic_array args) {
                return cast<double>(args.at(0)) - cast<double>(args.at(1));
            }),
            {make_calculation_request_with_value(dynamic(7.0)), remote_calc}));

    auto function_pointer_calc
        = make_calculation_request_with_lambda(make_lambda_calculation(
            make_function(dynamic_subtract),
            {make_calculation_request_with_value(dynamic(8.0)), lambda_calc}));

    REQUIRE(eval(function_pointer_calc) == dynamic(7.0));
}
