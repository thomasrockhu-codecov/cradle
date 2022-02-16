#ifndef CRADLE_EXTERNAL_API_IMPL_H
#define CRADLE_EXTERNAL_API_IMPL_H

#include <cradle/service/core.h>

namespace cradle {

namespace external {

struct api_service
{
    api_service(api_service_config const& config);
    ~api_service();

    cradle::service_core service_core_;
};

struct api_session
{
    api_service& service_;
    cradle::thinknode_session thinknode_session_;

    api_session(
        api_service& service, api_thinknode_session_config const& config);
    ~api_session();

    cradle::service_core&
    get_service_core()
    {
        return service_.service_core_;
    }
};

} // namespace external

} // namespace cradle

#endif
