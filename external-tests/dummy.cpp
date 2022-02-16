#include <cppcoro/sync_wait.hpp>

#include <cradle/external_api.h>
#include <cradle/utilities/testing.h>

TEST_CASE("dummy")
{
    cradle::external::api_service_config service_config{};

    cradle::external::api_thinknode_session_config session_config {
        .api_url = "https://mgh.thinknode.io/api/v1.0",
        .access_token = "xyz",
    };

    auto service{std::move(cradle::external::start_service(service_config))};
    auto session{std::move(cradle::external::start_thinknode_session(*service, session_config))};
    const std::string context_id{"context_id"};

    auto my_blob = cppcoro::sync_wait(
        cradle::external::get_iss_object(*session, context_id, "object_id"));
    REQUIRE(my_blob.size > 0);
}
