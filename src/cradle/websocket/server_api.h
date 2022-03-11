#ifndef CRADLE_WEBSOCKET_SERVER_API_H
#define CRADLE_WEBSOCKET_SERVER_API_H

#include <cppcoro/shared_task.hpp>
#include <cppcoro/task.hpp>
#include <cradle/core/api_types.hpp>
#include <cradle/service/core.h>
#include <cradle/thinknode/types.hpp>

namespace cradle {

cppcoro::shared_task<dynamic>
get_iss_object(
    service_core& service,
    thinknode_session session,
    string context_id,
    string object_id,
    bool ignore_upgrades = false);

cppcoro::shared_task<blob>
get_iss_blob(
    service_core& service,
    thinknode_session session,
    string context_id,
    string object_id,
    bool ignore_upgrades = false);

cppcoro::shared_task<api_type_info>
resolve_named_type_reference(
    service_core& service,
    thinknode_session session,
    string context_id,
    api_named_type_reference ref);

cppcoro::task<thinknode_app_version_info>
resolve_context_app(
    service_core& service,
    thinknode_session session,
    string context_id,
    string account,
    string app);

cppcoro::task<nil_t>
deeply_copy_iss_object(
    service_core& service,
    thinknode_session session,
    string source_bucket,
    string source_context_id,
    string destination_context_id,
    string object_id);

cppcoro::task<nil_t>
deeply_copy_calculation(
    service_core& service,
    thinknode_session session,
    string source_bucket,
    string source_context_id,
    string destination_context_id,
    string object_id);

cppcoro::task<nil_t>
deeply_copy_calculation(
    service_core& service,
    thinknode_session session,
    string source_bucket,
    string source_context_id,
    string destination_context_id,
    string object_id);

} // namespace cradle

#endif
