#include <cradle/external_api.h>
#include <cradle/typing/service/core.h>
#include <cradle/typing/utilities/testing.h>

using namespace cradle;

TEST_CASE("start_service", "[external]")
{
    cradle::external::api_service_config api_config{};
    REQUIRE_NOTHROW(cradle::external::start_service(api_config));
}
