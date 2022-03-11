#include <cradle/websocket/calculations.h>

#include <cradle/utilities/testing.h>

using namespace cradle;

namespace {

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

    REQUIRE(make_function_id(add) == make_function_id(add));
    REQUIRE(make_function_id(subtract) == make_function_id(subtract));
    REQUIRE(make_function_id(add) != make_function_id(subtract));

    REQUIRE(make_function_id(lambda_add) == make_function_id(lambda_add));
    REQUIRE(
        make_function_id(lambda_subtract)
        == make_function_id(lambda_subtract));
    REQUIRE(make_function_id(lambda_add) != make_function_id(lambda_subtract));

    REQUIRE(make_function_id(add) != make_function_id(lambda_add));
    REQUIRE(make_function_id(subtract) != make_function_id(lambda_subtract));

    captured_id captured(make_function_id(add));
    REQUIRE(captured.matches(make_function_id(add)));
    REQUIRE(!captured.matches(make_function_id(subtract)));
    REQUIRE(!captured.matches(make_function_id(lambda_add)));

    captured.capture(make_function_id(lambda_add));
    REQUIRE(captured.matches(make_function_id(lambda_add)));
    REQUIRE(!captured.matches(make_function_id(lambda_subtract)));
    REQUIRE(!captured.matches(make_function_id(add)));
}
