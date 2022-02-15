#ifndef CRADLE_EXTERNAL_API_IMPL_H
#define CRADLE_EXTERNAL_API_IMPL_H

#include <cradle/external_api.h>

namespace cradle {

namespace external {

class api_service_impl : public api_service_interface
{
public:
    api_service_impl(api_service_config const& config);

    // Also retrieves context_id for the realm, and does registration.
    // So a thinknode session in this API is a combination of thinknode_session (api_url, token),
    // and context_id, as appearing in CRADLE internally.
    std::unique_ptr<api_session_interface>
    start_thinknode_session(api_thinknode_session_config const& config) override;
};

class api_thinknode_session_impl : public api_session_interface
{
public:
    api_thinknode_session_impl(api_thinknode_session_config const& config);

    ~api_thinknode_session_impl();

    cppcoro::shared_task<blob>
    get_iss_object(
        string object_id,
        bool ignore_upgrades = false) override;

    cppcoro::shared_task<string>
    resolve_iss_object_to_immutable(
        string object_id,
        bool ignore_upgrades = false) override;

    cppcoro::shared_task<std::map<string, string>>
    get_iss_object_metadata(
        string object_id) override;

    // Returns object_id
    cppcoro::shared_task<string>
    post_iss_object(
        std::string schema,     // URL-type string
        blob object_data) override;

    // shared_task?
    cppcoro::task<>
    copy_iss_object(
        api_session_interface& destination,
        std::string object_id) override;

    // shared_task?
    cppcoro::task<>
    copy_calculation(
        api_session_interface& destination,
        std::string calculation_id) override;

    // Returns calculation_id
    // shared_task?
    // TODO get rid of calculation_request
    // TODO unify with perform_local_calculation()
    cppcoro::task<std::string>
    post_calculation(
        calculation_request request) override;

    cppcoro::shared_task<calculation_request>
    retrieve_calculation_request(
        std::string calculation_id) override;

    // Returns calculation_id; or blob?
    cppcoro::task<std::string>
    perform_local_calculation(
        calculation_request request) override;
};

} // namespace external

} // namespace cradle

#endif
