#include <cradle/thinknode/iam.h>

#include <cradle/core/monitoring.h>
#include <cradle/encodings/sha256_hash_id.h>
#include <cradle/io/http_requests.hpp>

namespace cradle {

namespace uncached {

cppcoro::task<thinknode_context_contents>
get_context_contents(
    service_core& service, thinknode_session session, string context_id)
{
    auto query = make_get_request(
        session.api_url + "/iam/contexts/" + context_id,
        {{"Authorization", "Bearer " + session.access_token},
         {"Accept", "application/json"}});

    auto response = co_await async_http_request(service, query);

    co_return from_dynamic<thinknode_context_contents>(
        parse_json_response(response));
}

} // namespace uncached

cppcoro::task<string>
get_context_id(service_core& service, thinknode_session session, string realm)
{
    auto query = make_get_request(
        session.api_url + "/iam/realms/" + realm + "/context",
        {{"Authorization", "Bearer " + session.access_token},
         {"Accept", "application/json"}});

    auto response = co_await async_http_request(service, query);

    co_return from_dynamic<id_response>(parse_json_response(response)).id;
}

cppcoro::shared_task<thinknode_context_contents>
get_context_contents(
    service_core& service, thinknode_session session, string context_id)
{
    auto cache_key = make_sha256_hashed_id(
        "get_context_contents", session.api_url, context_id);

    return fully_cached<thinknode_context_contents>(
        service, cache_key, [=, &service] {
            return uncached::get_context_contents(
                service, session, context_id);
        });
}

cppcoro::shared_task<string>
get_context_bucket(
    service_core& service, thinknode_session session, string context_id)
{
    auto context = co_await get_context_contents(
        service, session, std::move(context_id));
    co_return context.bucket;
}

} // namespace cradle
