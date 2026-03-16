#include "../vendor/catch2/catch.hpp"

#include "minidb/entry.hpp"

using namespace minidb;

TEST_CASE("Entry serialization roundtrip", "[entry]") {
    Entry e{1704067200, 1};
    auto bytes  = e.serialize();
    auto result = Entry::deserialize(bytes);

    REQUIRE(result.has_value());
    REQUIRE(result->timestamp == e.timestamp);
    REQUIRE(result->device_id == e.device_id);
}

TEST_CASE("Entry deserialize detects corruption", "[entry]") {
    Entry e{1704067200, 1};
    auto bytes = e.serialize();

    // Corrupt a byte
    bytes[2] ^= 0xFF;
    auto result = Entry::deserialize(bytes);

    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Entry serializes in little-endian", "[entry]") {
    Entry e{0x00659A80, 1};
    auto bytes = e.serialize();

    REQUIRE(bytes[0] == 0x80);
    REQUIRE(bytes[1] == 0x9A);
    REQUIRE(bytes[2] == 0x65);
    REQUIRE(bytes[3] == 0x00);
}

TEST_CASE("Entry SIZE is 6", "[entry]") {
    REQUIRE(Entry::SIZE == 6);
}