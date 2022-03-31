#ifndef CRADLE_SERVICE_TYPES
#define CRADLE_SERVICE_TYPES

#include <cradle/inner/caching/disk_cache.h>
#include <cradle/inner/caching/immutable.h>

namespace cradle {

api(struct)
struct service_immutable_cache_config
{
    // The maximum amount of memory to use for caching results that are no
    // longer in use, in bytes.
    integer unused_size_limit;
};

api(struct)
struct service_disk_cache_config
{
    optional<std::string> directory;
    integer size_limit;
};

api(struct)
struct service_config
{
    // config for the immutable memory cache
    omissible<service_immutable_cache_config> immutable_cache;

    // config for the disk cache
    omissible<service_disk_cache_config> disk_cache;

    // how many concurrent threads to use for request handling -
    // The default is one thread for each processor core.
    omissible<integer> request_concurrency;

    // how many concurrent threads to use for computing -
    // The default is one thread for each processor core.
    omissible<integer> compute_concurrency;

    // how many concurrent threads to use for HTTP requests
    omissible<integer> http_concurrency;
};

} // namespace cradle

#endif
