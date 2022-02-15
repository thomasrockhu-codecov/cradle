#include <cradle/external/external_api_impl.h>

namespace cradle {

namespace external {

std::unique_ptr<api_service_interface>
start_service(api_service_config const& config)
{
    return std::make_unique<api_service_impl>(config);
}


api_service_impl::api_service_impl(api_service_config const& config)
{
}

std::unique_ptr<api_session_interface>
api_service_impl::start_thinknode_session(api_thinknode_session_config const& config)
{
    return std::make_unique<api_thinknode_session_impl>(config);
}


api_thinknode_session_impl::api_thinknode_session_impl(api_thinknode_session_config const& config)
{
}

api_thinknode_session_impl::~api_thinknode_session_impl()
{
}

} // namespace external

} // namespace cradle
