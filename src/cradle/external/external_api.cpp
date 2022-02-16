#include <cradle/external/external_api_impl.h>

namespace cradle {

namespace external {

std::unique_ptr<api_service>
start_service(api_service_config const& config)
{
    return std::make_unique<api_service>();
}

std::unique_ptr<api_session>
start_thinknode_session(
    api_service& service, api_thinknode_session_config const& config)
{
    return std::make_unique<api_session>();
}

} // namespace external

} // namespace cradle
