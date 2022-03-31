#include <catch2/catch.hpp>

#include <cradle/inner/core/exception.h>

#include <cradle/inner/utilities/text.h>

using namespace cradle;

TEST_CASE("error info", "[inner][exception]")
{
    parsing_error error;
    error << parsed_text_info("asdf");

    REQUIRE(get_required_error_info<parsed_text_info>(error) == "asdf");

    try
    {
        get_required_error_info<expected_format_info>(error);
        FAIL("no exception thrown");
    }
    catch (missing_error_info& e)
    {
        get_required_error_info<error_info_id_info>(e);
        get_required_error_info<wrapped_exception_diagnostics_info>(e);
    }
}
