#ifndef CRADLE_SERVICE_INTERNALS_H
#define CRADLE_SERVICE_INTERNALS_H

#include <cppcoro/static_thread_pool.hpp>

#include <cradle/inner/service/internals.h>
#include <cradle/io/mock_http.h>
#include <cradle/thinknode/types.hpp>

namespace cradle {

namespace detail {

struct service_core_internals
{
    cppcoro::static_thread_pool http_pool;

    std::map<
        std::pair<string, thinknode_provider_image_info>,
        cppcoro::static_thread_pool>
        local_compute_pool;

    std::unique_ptr<mock_http_session> mock_http;
};

} // namespace detail

} // namespace cradle

#endif
