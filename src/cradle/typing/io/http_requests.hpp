#ifndef CRADLE_TYPING_IO_HTTP_REQUESTS_HPP
#define CRADLE_TYPING_IO_HTTP_REQUESTS_HPP

#include <cradle/inner/fs/types.h>

#include <memory>

// This file defines a low-level facility for doing authenticated HTTP
// requests.

namespace cradle {

struct progress_reporter_interface;
struct check_in_interface;

// HTTP headers are specified as a mapping from field names to values.
typedef std::map<string, string> http_header_list;

// The body of an HTTP request is a blob.
typedef blob http_body;

// supported HTTP request methods
api(enum internal)
enum class http_request_method
{
    POST,
    GET,
    PUT,
    DELETE,
    PATCH,
    HEAD
};

api(struct internal)
struct http_request
{
    http_request_method method;
    string url;
    http_header_list headers;
    blob body;
    optional<string> socket;
};

// Construct a GET request (in a convenient way).
inline http_request
make_get_request(string url, http_header_list headers)
{
    return http_request{
        http_request_method::GET,
        std::move(url),
        std::move(headers),
        http_body(),
        none};
}

// Construct a general HTTP request.
inline http_request
make_http_request(
    http_request_method method,
    string url,
    http_header_list headers,
    http_body body)
{
    return http_request{
        method, std::move(url), std::move(headers), std::move(body), none};
}

// Redact an HTTP request.
http_request
redact_request(http_request request);

api(struct internal)
struct http_response
{
    int status_code;
    http_header_list headers;
    blob body;
};

// Parse a http_response as a JSON value.
dynamic
parse_json_response(http_response const& response);

// Parse a http_response as a MessagePack value.
dynamic
parse_msgpack_response(http_response const& response);

// Make a successful (200) HTTP response with the given body.
http_response
make_http_200_response(string body);

// This exception indicates a general failure in the HTTP request
// system (e.g., a failure to initialize).
CRADLE_DEFINE_EXCEPTION(http_request_system_error)

// This exception indicates that a failure occurred in the processing
// of a HTTP request that precluded getting a response from the server
// (e.g., the server couldn't be reached).
CRADLE_DEFINE_EXCEPTION(http_request_failure)
// This exception also provides internal_error_message_info.
CRADLE_DEFINE_ERROR_INFO(http_request, attempted_http_request)

// This exception indicates that an HTTP request was resolved but
// resulted in a status code outside the 2xx range. The full response
// is included.
CRADLE_DEFINE_EXCEPTION(bad_http_status_code)
// This exception also provides attempted_http_request_info.
CRADLE_DEFINE_ERROR_INFO(http_response, http_response)

// http_request_system provides global initialization and shutdown of the HTTP
// request system. Exactly one of these objects must be instantiated by the
// application, and its scope must dominate the scope of all http_connection
// objects.

struct http_request_system : noncopyable
{
    http_request_system();
    ~http_request_system();
};

// http_connection provides a network connection over which HTTP requests can
// be made.

struct http_connection_interface
{
    // Perform an HTTP request and return the response.
    // Since this may take a long time to complete, monitoring is provided.
    // Accurate progress reporting relies on the web server providing the size
    // of the response.
    virtual http_response
    perform_request(
        check_in_interface& check_in,
        progress_reporter_interface& reporter,
        http_request const& request)
        = 0;
};

struct http_connection_impl;

struct http_connection : http_connection_interface
{
    http_connection(http_request_system& system);
    ~http_connection();

    http_connection(http_connection&&);
    http_connection&
    operator=(http_connection&&);

    http_response
    perform_request(
        check_in_interface& check_in,
        progress_reporter_interface& reporter,
        http_request const& request) override;

 private:
    std::unique_ptr<http_connection_impl> impl_;
};

} // namespace cradle

#endif
