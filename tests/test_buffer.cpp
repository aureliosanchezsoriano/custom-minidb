#include "../vendor/catch2/catch.hpp"
#include "minidb/buffer.hpp"
#include <filesystem>
#include <thread>

using namespace minidb;

static const std::filesystem::path TEST_FILE = "test_buffer.bin";

struct BufferFixture {
    BufferFixture()  { std::filesystem::remove(TEST_FILE); }
    ~BufferFixture() { std::filesystem::remove(TEST_FILE); }
};

TEST_CASE_METHOD(BufferFixture, "Buffer flushes when full", "[buffer]") {
    Pager     p(TEST_FILE);
    Buffer<>  b(p, Buffer<>::MIN_SIZE);

    for (std::size_t i = 0; i < Buffer<>::MIN_SIZE; ++i)
        REQUIRE(b.push(Entry{static_cast<uint32_t>(1704067200 + i),
                             static_cast<uint8_t>((i % 254) + 1)}));

    REQUIRE(p.entry_count() == Buffer<>::MIN_SIZE);
    REQUIRE(b.size() == 0);
}

TEST_CASE_METHOD(BufferFixture, "Buffer manual flush", "[buffer]") {
    Pager    p(TEST_FILE);
    Buffer<> b(p);

    b.push(Entry{1704067200, 1});
    b.push(Entry{1704067201, 2});

    REQUIRE(b.size() == 2);
    REQUIRE(b.flush());
    REQUIRE(b.size() == 0);
    REQUIRE(p.entry_count() == 2);
}

TEST_CASE_METHOD(BufferFixture, "Buffer flushes after timeout", "[buffer]") {
    Pager     p(TEST_FILE);
    Buffer<1> b(p, Buffer<1>::MIN_SIZE);

    b.push(Entry{1704067200, 1});
    b.push(Entry{1704067201, 2});

    std::this_thread::sleep_for(std::chrono::seconds(2));
    b.push(Entry{1704067202, 3});  // este push activa el flush por timeout

    REQUIRE(p.entry_count() >= 2);
}

TEST_CASE_METHOD(BufferFixture, "Buffer grows under high load", "[buffer]") {
    Pager    p(TEST_FILE);
    Buffer<> b(p, Buffer<>::MIN_SIZE);

    const std::size_t initial_capacity = b.capacity();

    for (int round = 0; round < 3; ++round)
        for (std::size_t i = 0; i < initial_capacity; ++i)
            b.push(Entry{static_cast<uint32_t>(1704067200 + i), 1});

    REQUIRE(b.capacity() > initial_capacity);
}