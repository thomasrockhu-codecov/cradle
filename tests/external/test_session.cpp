#include <utility>

#include "test_session.h"

using namespace cradle;

external_test_session::external_test_session()
{
    cradle::external::api_service_config service_config{};
    service_ = std::move(cradle::external::start_service(service_config));
    cradle::external::api_thinknode_session_config session_config{
        .api_url = "https://mgh.thinknode.io/api/v1.0",
        .access_token = "xyz",
    };
    session_ = std::move(
        cradle::external::start_session(*service_, session_config));
}

mock_http_session&
external_test_session::enable_http_mocking()
{
    auto& inner_service{session_->get_service_core()};
    init_test_service(inner_service);
    return cradle::enable_http_mocking(inner_service);
}
