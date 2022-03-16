#ifndef CRADLE_WEBSOCKET_LOCAL_CALCS_H
#define CRADLE_WEBSOCKET_LOCAL_CALCS_H

#include <cradle/service/core.h>
#include <cradle/thinknode/types.hpp>

namespace cradle {

cppcoro::task<dynamic>
perform_local_function_calc(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    string const& account,
    string const& app,
    string const& name,
    std::vector<dynamic> args);

cppcoro::task<dynamic>
coerce_local_calc_result(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    thinknode_type_info const& schema,
    dynamic value);

} // namespace cradle

#endif
