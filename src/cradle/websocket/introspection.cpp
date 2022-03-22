#include <chrono>

#include "cradle/introspection/tasklet_info.h"
#include "cradle/websocket/introspection.h"

namespace cradle {

template<typename TimePoint>
static auto
to_millis(TimePoint time_point) -> TimePoint::rep
{
    auto duration = duration_cast<std::chrono::milliseconds>(
        time_point.time_since_epoch());
    return duration.count();
}

static tasklet_msg_event
make_tasklet_msg_event(tasklet_event const& event)
{
    return make_tasklet_msg_event(
        to_millis(event.when()), to_string(event.what()), event.details());
}

static tasklet_overview
make_tasklet_overview(tasklet_info const& info)
{
    omissible<integer> client_id;
    if (info.have_client())
    {
        client_id = info.client_id();
    }
    std::vector<tasklet_msg_event> msg_events;
    for (auto const& e : info.events())
    {
        msg_events.push_back(make_tasklet_msg_event(e));
    }
    return make_tasklet_overview(
        info.pool_name(),
        info.own_id(),
        client_id,
        info.title(),
        std::move(msg_events));
}

introspection_status_response
make_introspection_status_response(bool include_finished)
{
    std::vector<tasklet_overview> overviews;
    for (auto t : get_tasklet_infos(include_finished))
    {
        overviews.push_back(make_tasklet_overview(t));
    }
    auto now = std::chrono::system_clock::now();
    return make_introspection_status_response(
        to_millis(now), std::move(overviews));
}

void
introspection_control(cradle::introspection_control_request const& request)
{
    switch (get_tag(request))
    {
        case introspection_control_request_tag::ON_OFF: {
            bool enabled = as_on_off(request);
            introspection_on_off(enabled);
            introspection_logging_on_off(enabled);
            break;
        }
        case introspection_control_request_tag::CLEAR_ADMIN:
            introspection_clear_all_info();
            break;
        default:
            CRADLE_THROW(
                invalid_enum_value()
                << enum_id_info("introspection_control_tag")
                << enum_value_info(static_cast<int>(get_tag(request))));
            break;
    }
}

} // namespace cradle
