#include <cradle/thinknode/iam.h>

#include <cradle/core/monitoring.h>
#include <cradle/encodings/sha256_hash_id.h>
#include <cradle/io/http_requests.hpp>
#include <cradle/thinknode/utilities.h>

namespace cradle {

namespace uncached {

cppcoro::task<thinknode_context_contents>
get_context_contents(thinknode_request_context trc, string context_id)
{
    auto query = make_get_request(
        trc.session.api_url + "/iam/contexts/" + context_id,
        {{"Authorization", "Bearer " + trc.session.access_token},
         {"Accept", "application/json"}});

    auto response = co_await async_http_request(trc, query);

    co_return from_dynamic<thinknode_context_contents>(
        parse_json_response(response));
}

} // namespace uncached

cppcoro::task<string>
get_context_id(thinknode_request_context trc, string realm)
{
    auto query = make_get_request(
        trc.session.api_url + "/iam/realms/" + realm + "/context",
        {{"Authorization", "Bearer " + trc.session.access_token},
         {"Accept", "application/json"}});

    auto response = co_await async_http_request(trc, query);

    co_return from_dynamic<id_response>(parse_json_response(response)).id;
}

cppcoro::shared_task<thinknode_context_contents>
get_context_contents(thinknode_request_context trc, string context_id)
{
    auto cache_key = make_sha256_hashed_id(
        "get_context_contents", trc.session.api_url, context_id);

    return fully_cached<thinknode_context_contents>(
        trc.service, cache_key, [=] {
            return uncached::get_context_contents(trc, context_id);
        });
}

cppcoro::shared_task<string>
get_context_bucket(thinknode_request_context trc, string context_id)
{
    auto context = co_await get_context_contents(trc, std::move(context_id));
    co_return context.bucket;
}

} // namespace cradle
