#include <cstring>

#include <cppcoro/sync_wait.hpp>

#include <cradle/external_api.h>
#include <cradle/io/mock_http.h>
#include <cradle/utilities/testing.h>

using namespace cradle;

TEST_CASE("get_iss_object", "[external]")
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
    mock_http.set_script(
        {{make_get_request(
              "https://mgh.thinknode.io/api/v1.0/iss/my_object_id/immutable?context=123&ignore_upgrades=false",
              {{"Authorization", "Bearer xyz"},
               {"Accept", "application/json"}}),
          make_http_200_response("{ \"id\": \"my_immutable_id\" }")},
         {make_get_request(
              "https://mgh.thinknode.io/api/v1.0/iss/immutable/my_immutable_id?context=123",
              {{"Authorization", "Bearer xyz"},
               {"Accept", "application/octet-stream"}}),
          make_http_200_response(blob_value.c_str())}});

    // Initial request: nothing cached yet, so all responses should come via
    // (mock) HTTP
    auto blob0 = cppcoro::sync_wait(cradle::external::get_iss_object(
        *session, context_id, "my_object_id"));

    REQUIRE(blob0.size == blob_value.size());
    REQUIRE(strncmp(blob0.data, blob_value.c_str(), blob_value.size()) == 0);

    REQUIRE(mock_http.is_complete());
    REQUIRE(mock_http.is_in_order());

    // Same request, no HTTP allowed: all responses should be in the cache
    auto blob1 = cppcoro::sync_wait(cradle::external::get_iss_object(
        *session, context_id, "my_object_id"));

    REQUIRE(blob1.size == blob_value.size());
    REQUIRE(strncmp(blob1.data, blob_value.c_str(), blob_value.size()) == 0);
}
