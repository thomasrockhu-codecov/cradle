#include <iomanip>
#include <sstream>

#include <cradle/introspection/tasklet_impl.h>
#include <cradle/introspection/tasklet_info.h>

namespace cradle {

std::string
to_string(tasklet_event_type what)
{
    static const char* const strings[] = {
        "scheduled",
        "running",
        "before co_await",
        "after co_await",
        "finished",
    };
    int index = static_cast<int>(what);
    return std::string{strings[index]};
}

tasklet_event::tasklet_event(tasklet_event_type what) : tasklet_event{what, ""}
{
}

tasklet_event::tasklet_event(
    tasklet_event_type what, std::string const& details)
    : when_{std::chrono::system_clock::now()}, what_{what}, details_{details}
{
}

tasklet_info::tasklet_info(tasklet_impl const& impl)
    : own_id_{impl.own_id()},
      pool_name_{impl.pool_name()},
      title_{impl.title()},
      client_id_{impl.client() ? impl.client()->own_id() : -1}
{
    for (auto const& opt_evt : impl.optional_events())
    {
        if (opt_evt)
        {
            events_.push_back(*opt_evt);
        }
    }
}

std::vector<tasklet_info>
get_tasklet_infos(bool include_finished)
{
    return tasklet_admin::instance().get_tasklet_infos(include_finished);
}

void
introspection_on_off(bool enabled)
{
    return tasklet_admin::instance().on_off(enabled);
}

void
introspection_logging_on_off(bool enabled)
{
    return tasklet_admin::instance().logging_on_off(enabled);
}

void
introspection_clear_all_info()
{
    return tasklet_admin::instance().clear_all_info();
}

} // namespace cradle
