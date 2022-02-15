// include/cradle/external_api.h

#ifndef INCLUDE_CRADLE_EXTERNAL_API_H
#define INCLUDE_CRADLE_EXTERNAL_API_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include <cppcoro/shared_task.hpp>
#include <cppcoro/task.hpp>

#include <cradle/core/type_definitions.h>  // blob
#include <cradle/thinknode/types.hpp>  // calculation_request

namespace cradle {

namespace external {

// Failures lead to thrown exceptions

struct api_service_config
{
    // The maximum amount of memory to use for caching results that are no
    // longer in use, in bytes.
    std::optional<std::size_t> memory_cache_unused_size_limit;

    // config for the disk cache
    std::optional<std::string> disk_cache_directory;
    std::optional<std::size_t> disk_cache_size_limit;

    // how many concurrent threads to use for request handling -
    // The default is one thread for each processor core.
    std::optional<int> request_concurrency;

    // how many concurrent threads to use for computing -
    // The default is one thread for each processor core.
    std::optional<int> compute_concurrency;

    // how many concurrent threads to use for HTTP requests
    std::optional<int> http_concurrency;
};

struct api_thinknode_session_config
{
    std::string api_url;
    std::string access_token;
    std::string realm_name;
};

struct api_service_interface;
struct api_session_interface;

// The client owns the service, and is responsible for stopping it by letting the unique_ptr go out of scope
// (or doing a service.reset()).
std::unique_ptr<api_service_interface>
start_service(api_service_config const& config);

struct api_service_interface
{
    // Note: must not throw
    virtual ~api_service_interface() = default;

    // Also retrieves context_id for the realm, and does registration.
    // So a thinknode session in this API is a combination of thinknode_session (api_url, token),
    // and context_id, as appearing in CRADLE internally.
    virtual std::unique_ptr<api_session_interface>
    start_thinknode_session(api_thinknode_session_config const& config) = 0;
};

struct api_session_interface
{
    // Note: must not throw
    virtual ~api_session_interface() = default;

    virtual cppcoro::shared_task<blob>
    get_iss_object(
        string object_id,
        bool ignore_upgrades = false) = 0;

    virtual cppcoro::shared_task<string>
    resolve_iss_object_to_immutable(
        string object_id,
        bool ignore_upgrades = false) = 0;

    virtual cppcoro::shared_task<std::map<string, string>>
    get_iss_object_metadata(
        string object_id) = 0;

    // Returns object_id
    virtual cppcoro::shared_task<string>
    post_iss_object(
        std::string schema,     // URL-type string
        blob object_data) = 0;

    // shared_task?
    virtual cppcoro::task<>
    copy_iss_object(
        api_session_interface& destination,
        std::string object_id) = 0;

    // shared_task?
    virtual cppcoro::task<>
    copy_calculation(
        api_session_interface& destination,
        std::string calculation_id) = 0;

    // Returns calculation_id
    // shared_task?
    // TODO get rid of calculation_request
    // TODO unify with perform_local_calculation()
    virtual cppcoro::task<std::string>
    post_calculation(
        calculation_request request) = 0;

    virtual cppcoro::shared_task<calculation_request>
    retrieve_calculation_request(
        std::string calculation_id) = 0;

    // Returns calculation_id; or blob?
    virtual cppcoro::task<std::string>
    perform_local_calculation(
        calculation_request request) = 0;
};

} // namespace external

} // namespace cradle

#endif
