#include <cradle/external_api.h>
#include <cradle/service/core.h>
#include <cradle/utilities/testing.h>

using namespace cradle;

TEST_CASE("start_session", "[external]")
{
    cradle::external::api_service_config service_config{};
    auto service = cradle::external::start_service(service_config);
    cradle::external::api_thinknode_session_config session_config{
        .api_url = "https://mgh.thinknode.io/api/v1.0",
        .access_token = "xyz",
    };
    auto session = cradle::external::start_session(*service, session_config);

    REQUIRE(session);

    auto& tn_session = session->get_thinknode_session();
    REQUIRE(tn_session.api_url == "https://mgh.thinknode.io/api/v1.0");
    REQUIRE(tn_session.access_token == "xyz");
}
