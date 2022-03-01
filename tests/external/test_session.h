#ifndef TESTS_EXTERNAL_TEST_SESSION_H
#define TESTS_EXTERNAL_TEST_SESSION_H

#include <cradle/external_api.h>
#include <cradle/io/mock_http.h>
#include <memory>

class external_test_session
{
    std::unique_ptr<cradle::external::api_service> service_;
    std::unique_ptr<cradle::external::api_session> session_;

 public:
    external_test_session();

    cradle::external::api_session&
    api_session()
    {
        return *session_;
    }

    cradle::mock_http_session&
    enable_http_mocking();
};

#endif
