#include <cradle/thinknode/apm.h>

#include <cradle/core/monitoring.h>
#include <cradle/encodings/sha256_hash_id.h>
#include <cradle/io/http_requests.hpp>
#include <cradle/thinknode/utilities.h>

namespace cradle {

namespace uncached {

cppcoro::task<thinknode_app_version_info>
get_app_version_info(
    thinknode_request_context trc, string account, string app, string version)
{
    auto query = make_get_request(
        trc.session.api_url + "/apm/apps/" + account + "/" + app + "/versions/"
            + version + "?include_manifest=true",
        {{"Authorization", "Bearer " + trc.session.access_token},
         {"Accept", "application/json"}});

    auto response = co_await async_http_request(trc, query);
    co_return from_dynamic<thinknode_app_version_info>(
        parse_json_response(response));
}

} // namespace uncached

cppcoro::shared_task<thinknode_app_version_info>
get_app_version_info(
    thinknode_request_context trc, string account, string app, string version)
{
    auto cache_key = make_sha256_hashed_id(
        "get_app_version_info", trc.session.api_url, account, app, version);

    return fully_cached<thinknode_app_version_info>(
        trc.service, cache_key, [=] {
            return uncached::get_app_version_info(trc, account, app, version);
        });
}

} // namespace cradle
