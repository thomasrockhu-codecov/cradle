#include <cradle/external_api.h>

namespace cradle {

// In websocket/server.cpp
// TODO put in a header file
extern cppcoro::shared_task<blob>
get_iss_blob(
    cradle::service_core& service,
    cradle::thinknode_session session,
    std::string context_id,
    std::string object_id,
    bool ignore_upgrades);

cppcoro::task<std::string>
post_calculation_piecewise(
    service_core& core,
    thinknode_session session,
    string context_id,
    calculation_request request);

namespace external {

std::unique_ptr<api_service>
start_service(api_service_config const& config)
{
    return std::make_unique<api_service>(config);
}

cradle::service_config
make_service_config(api_service_config const& config)
{
    // TODO
    return cradle::service_config{};
}

api_service::api_service(api_service_config const& config)
    : service_core_{make_service_config(config)}
{
}

std::unique_ptr<api_session>
start_session(api_service& service, api_thinknode_session_config const& config)
{
    return std::make_unique<api_session>(service, config);
}

api_session::api_session(
    api_service& service, api_thinknode_session_config const& config)
    : service_{service},
      thinknode_session_{
          cradle::make_thinknode_session(config.api_url, config.access_token)}
{
}

cppcoro::shared_task<blob>
get_iss_object(
    api_session& session,
    std::string context_id,
    std::string object_id,
    bool ignore_upgrades)
{
    return get_iss_blob(
        session.get_service_core(),
        session.thinknode_session_,
        context_id,
        object_id,
        ignore_upgrades);
}

cppcoro::task<std::string>
post_calculation(
    api_session& session, std::string context_id, calculation_request request)
{
    return post_calculation_piecewise(
        session.get_service_core(),
        session.thinknode_session_,
        context_id,
        request);
}

} // namespace external

} // namespace cradle
