#include <cradle/websocket/calculations.h>

#include <cradle/utilities/testing.h>

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
