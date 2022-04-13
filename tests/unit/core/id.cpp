#include <cradle/inner/core/id.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <unordered_map>

#include <boost/lexical_cast.hpp>
#include <catch2/catch.hpp>

#include "../../inner/support/ids.h"
#include <cradle/typing/encodings/sha256_hash_id.h>

using namespace cradle;

TEST_CASE("sha256_hashed_id", "[id]")
{
    test_different_ids(
        make_sha256_hashed_id("token", 0), make_sha256_hashed_id("token", 1));
    auto as_string
        = boost::lexical_cast<std::string>(make_sha256_hashed_id("token", 0));
    REQUIRE(as_string.length() == 64);
    REQUIRE(std::all_of(as_string.begin(), as_string.end(), [](char c) {
        return std::isxdigit(c);
    }));
}

TEST_CASE("captured sha256_hashed_id", "[id]")
{
    auto captured = make_captured_sha256_hashed_id(std::string("xyz"), 87);
    auto made = make_sha256_hashed_id(std::string("xyz"), 87);
    REQUIRE(*captured == made);
}
