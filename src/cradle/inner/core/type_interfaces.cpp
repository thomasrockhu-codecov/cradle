#include <cradle/inner/core/type_interfaces.h>

#include <cstring>

#include <boost/functional/hash.hpp>

namespace cradle {

bool
operator==(blob const& a, blob const& b)
{
    return a.size() == b.size()
           && (a.data() == b.data()
               || std::memcmp(a.data(), b.data(), a.size()) == 0);
}

bool
operator<(blob const& a, blob const& b)
{
    return a.size() < b.size()
           || (a.size() == b.size() && a.data() != b.data()
               && std::memcmp(a.data(), b.data(), a.size()) < 0);
}

size_t
hash_value(blob const& x)
{
    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(x.data());
    return boost::hash_range(bytes, bytes + x.size());
}

blob
make_static_blob(std::byte const* data, size_t size)
{
    return blob(
        std::shared_ptr<std::byte const>(data, [](std::byte const*) {}), size);
}

blob
make_string_literal_blob(char const* data)
{
    return make_static_blob(as_bytes(data), strlen(data));
}

blob
make_blob(std::string s)
{
    // This is a little roundabout, but it seems like the most reasonable way
    // to ensure that a) the string contents don't move if the blob is moved
    // and b) the string contents aren't actually copied if they're large.
    size_t size = s.size();
    auto shared_string = std::make_shared<std::string>(std::move(s));
    char const* data = shared_string->data();
    return make_blob(std::move(shared_string), as_bytes(data), size);
}

blob
make_blob(byte_vector v)
{
    size_t size = v.size();
    auto shared_vector = std::make_shared<byte_vector>(std::move(v));
    char const* data = reinterpret_cast<char const*>(shared_vector->data());
    return make_blob(std::move(shared_vector), as_bytes(data), size);
}

} // namespace cradle
