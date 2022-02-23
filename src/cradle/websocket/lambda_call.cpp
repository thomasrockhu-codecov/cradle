#include <cradle/encodings/msgpack.h>
#include <cradle/encodings/sha256_hash_id.h>
#include <cradle/websocket/lambda_call.h>

using namespace std::literals::string_literals;

namespace cradle {

cppcoro::task<blob>
do_lambda_call_uncached(lambda_call const& my_call)
{
    dynamic result = my_call.func(my_call.args);
    co_return value_to_msgpack_blob(result);
}

cppcoro::task<string>
get_immutable_id_uncached(string immutable_id)
{
    co_return immutable_id;
}

cppcoro::shared_task<std::string>
do_lambda_call_cached(
    service_core& service,
    thinknode_session session,
    string context_id,
    lambda_call const& my_call)
{
    // TODO add func ptr, args in the mix
    const string immutable_id{"my_immutable_id_for_lambda"};

    // Calculate the blob and store it in the cache where it can be retrieved
    // using immutable_id
    auto cache_key0 = make_sha256_hashed_id(
        "retrieve_immutable_blob"s, session.api_url, immutable_id);
    auto ignored_blob = co_await cached<blob>(
        service, cache_key0, [&] { return do_lambda_call_uncached(my_call); });

    // Store the object_id -> immutable_id translation in the cache
    const string object_id{immutable_id + "_obj"};
    auto cache_key1 = make_sha256_hashed_id(
        "resolve_iss_object_to_immutable"s,
        session.api_url,
        context_id,
        object_id);
    // #170 Putting local_lambda inside the co_await leads to a double free,
    // probably due to a compiler bug.
    auto local_lambda
        = [=] { return get_immutable_id_uncached(immutable_id); };
    auto ignored_object_id
        = co_await cached<string>(service, cache_key1, local_lambda);

    co_return object_id;
}

} // namespace cradle
