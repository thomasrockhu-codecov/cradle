#include <cppcoro/sync_wait.hpp>

#include <cradle/external_api.h>
#include <cradle/utilities/testing.h>

using namespace cradle;
using namespace cradle::external;

TEST_CASE("dummy")
{
    api_service_config service_config{};

    api_thinknode_session_config session_config {
        .api_url = "https://mgh.thinknode.io/api/v1.0",
        .access_token = "xyz",
        .realm_name = "realm"
    };

    auto service{std::move(start_service(service_config))};
    auto session{std::move(service->start_thinknode_session(session_config))};

    blob my_blob = cppcoro::sync_wait(
        session->get_iss_object("object_id"));
    REQUIRE(my_blob.size > 0);
}
