#ifndef CRADLE_WEBSOCKET_HYBRID_CALCS_H
#define CRADLE_WEBSOCKET_HYBRID_CALCS_H

#include <cradle/service/core.h>
#include <cradle/websocket/types.hpp>

namespace cradle {

cppcoro::task<dynamic>
resolve_hybrid_calc_to_value(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    hybrid_calculation_request request);

cppcoro::task<std::string>
resolve_hybrid_calc_to_iss_object(
    service_core& service,
    thinknode_session const& session,
    string const& context_id,
    hybrid_calculation_request request);

} // namespace cradle

#endif
