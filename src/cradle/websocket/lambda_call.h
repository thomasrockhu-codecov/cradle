#ifndef CRADLE_WEBSOCKET_LAMBDA_CALL_H
#define CRADLE_WEBSOCKET_LAMBDA_CALL_H

#include <string>

#include <cppcoro/shared_task.hpp>

#include <cradle/service/core.h>
#include <cradle/thinknode/types.hpp>

namespace cradle {

cppcoro::shared_task<std::string>
do_lambda_call_cached(
    service_core& service,
    thinknode_session session,
    std::string context_id,
    lambda_call const& my_call);

} // namespace cradle

#endif
