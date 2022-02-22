#include <chrono>
#include <cstring>
#include <thread>

#include <cppcoro/sync_wait.hpp>

#include <cradle/encodings/msgpack.h>
#include <cradle/external_api.h>
#include <cradle/io/mock_http.h>
#include <cradle/utilities/testing.h>

using namespace cradle;

static void
test_lambda_call(lambda_call const& my_lambda_call, int expected)
{
    cradle::external::api_service_config service_config{};
    cradle::external::api_thinknode_session_config session_config{
        .api_url = "https://mgh.thinknode.io/api/v1.0",
        .access_token = "xyz",
    };
    auto service{std::move(cradle::external::start_service(service_config))};
    auto session{
        std::move(cradle::external::start_session(*service, session_config))};
    const std::string context_id{"123"};
    const std::string blob_value{"my_blob_value"};

    auto& inner_service{session->get_service_core()};
    init_test_service(inner_service);
    auto& mock_http = enable_http_mocking(inner_service);
    mock_http.set_script({});

    auto my_calc_request
        = make_calculation_request_with_lambda(my_lambda_call);

    auto calc_id0 = cppcoro::sync_wait(cradle::external::post_calculation(
        *session, context_id, my_calc_request));

    blob blob0 = cppcoro::sync_wait(
        cradle::external::get_iss_object(*session, context_id, calc_id0));

    // TODO pass blob's owner to parse_msgpack_value()
    // TODO don't have so many reinterpret_cast's
    auto blob0_data = reinterpret_cast<uint8_t const*>(blob0.data());
    dynamic dynamic0 = parse_msgpack_value(blob0_data, blob0.size());

    REQUIRE(dynamic0.type() == value_type::INTEGER);
    REQUIRE(cast<integer>(dynamic0) == expected);

    REQUIRE(mock_http.is_complete());
    REQUIRE(mock_http.is_in_order());

    // This should not lead to another lambda call
    auto calc_id1 = cppcoro::sync_wait(cradle::external::post_calculation(
        *session, context_id, my_calc_request));
    REQUIRE(calc_id1 == calc_id0);
}

static int
multiply(cradle::dynamic_array const& args)
{
    int result = 1;
    for (auto arg : args)
    {
        int factor{};
        from_dynamic(&factor, arg);
        result *= factor;
    }
    return result;
}

static dynamic
scrabble_func(cradle::dynamic_array const& args)
{
    return dynamic(static_cast<integer>(multiply(args) - 12));
}

TEST_CASE("raw_function_call", "[external]")
{
    std::vector<dynamic> args{
        dynamic(static_cast<integer>(6)), dynamic(static_cast<integer>(9))};
    lambda_call my_lambda_call{scrabble_func, args};
    test_lambda_call(my_lambda_call, 42);
}

TEST_CASE("lambda_call", "[external]")
{
    std::vector<dynamic> args{
        dynamic(static_cast<integer>(6)), dynamic(static_cast<integer>(9))};
    auto scrabble_lambda = [](cradle::dynamic_array const& args) -> dynamic {
        return dynamic(static_cast<integer>(multiply(args)));
    };
    lambda_call my_lambda_call{scrabble_lambda, args};
    test_lambda_call(my_lambda_call, 54);
}

struct ScrabbleClass
{
    dynamic
    operator()(cradle::dynamic_array const& args) const
    {
        return dynamic(static_cast<integer>(multiply(args) + 12));
    }
};

TEST_CASE("object_function_call", "[external]")
{
    std::vector<dynamic> args{
        dynamic(static_cast<integer>(6)), dynamic(static_cast<integer>(9))};
    lambda_call my_lambda_call{ScrabbleClass(), args};
    test_lambda_call(my_lambda_call, 66);
}
