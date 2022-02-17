#ifndef INCLUDE_CRADLE_EXTERNAL_API_H
#define INCLUDE_CRADLE_EXTERNAL_API_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include <cppcoro/shared_task.hpp>
#include <cppcoro/task.hpp>

#include <cradle/core/type_definitions.h> // blob
#include <cradle/thinknode/types.hpp> // calculation_request

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
};

struct api_service;
struct api_session;

// The client owns the service, and is responsible for stopping it by letting
// the unique_ptr go out of scope (or doing a service.reset()).
std::unique_ptr<api_service>
start_service(api_service_config const& config);

std::unique_ptr<api_session>
start_session(
    api_service& service, api_thinknode_session_config const& config);

cppcoro::task<std::string>
get_context_id(api_session& session, std::string realm);

cppcoro::shared_task<blob>
get_iss_object(
    api_session& session,
    std::string context_id,
    std::string object_id,
    bool ignore_upgrades = false);

cppcoro::shared_task<std::string>
resolve_iss_object_to_immutable(
    api_session& session,
    std::string context_id,
    std::string object_id,
    bool ignore_upgrades = false);

cppcoro::shared_task<std::map<std::string, std::string>>
get_iss_object_metadata(
    api_session& session, std::string context_id, std::string object_id);

// Returns object_id
cppcoro::shared_task<std::string>
post_iss_object(
    api_session& session,
    std::string context_id,
    std::string schema, // URL-type string
    blob object_data);

// shared_task?
cppcoro::task<>
copy_iss_object(
    api_session& session,
    std::string source_context_id,
    std::string destination_context_id,
    std::string object_id);

// shared_task?
cppcoro::task<>
copy_calculation(
    api_session& session,
    std::string source_context_id,
    std::string destination_context_id,
    std::string calculation_id);

// Returns calculation_id (also for local calculation!)
// shared_task?
cppcoro::task<std::string>
post_calculation(
    api_session& session, std::string context_id, calculation_request request);

cppcoro::shared_task<calculation_request>
retrieve_calculation_request(
    api_session& session, std::string context_id, std::string calculation_id);

} // namespace external

} // namespace cradle

// Client cannot just #include external_api.h because unique_ptr's deleter
// needs complete types. Use shared_ptr? Or some non-std:: type-erasing unique
// pointer?
#include <cradle/external/external_api_impl.h>

#endif
