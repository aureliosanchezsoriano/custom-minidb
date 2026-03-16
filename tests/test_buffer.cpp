#include "../vendor/catch2/catch.hpp"

#include "minidb/buffer.hpp"
#include "minidb/pager.hpp"
#include "minidb/entry.hpp"

#include <filesystem>


static const std::filesystem::path TEST_FILE = "test_buffer.bin";

struct BufferFixture {
    BufferFixture()  { std::filesystem::remove(TEST_FILE); }
    ~BufferFixture() { std::filesystem::remove(TEST_FILE); }
};

// Clock for timeout tests
struct TestClock {
    using duration   = std::chrono::steady_clock::duration;
    using time_point = std::chrono::time_point<TestClock>;

    static time_point now() { return current_time; }
    static void advance(std::chrono::seconds s) { current_time += s; }
    static void reset() { current_time = time_point{}; }

    static inline time_point current_time{};
};

TEST_CASE_METHOD(BufferFixture, "Buffer flushes when full", "[buffer]") {
    minidb::Pager     p(TEST_FILE);
    minidb::Buffer<>  b(p, minidb::Buffer<>::MIN_SIZE);

    for (std::size_t i = 0; i < minidb::Buffer<>::MIN_SIZE; ++i)
        REQUIRE(b.push(minidb::Entry{static_cast<uint32_t>(1704067200 + i),
                             static_cast<uint8_t>((i % 254) + 1)}));

    REQUIRE(p.entry_count() == minidb::Buffer<>::MIN_SIZE);
    REQUIRE(b.size() == 0);
}

TEST_CASE_METHOD(BufferFixture, "Buffer manual flush", "[buffer]") {
    minidb::Pager    p(TEST_FILE);
    minidb::Buffer<> b(p);

    (void)b.push(minidb::Entry{1704067200, 1});
    (void)b.push(minidb::Entry{1704067201, 2});

    REQUIRE(b.size() == 2);
    REQUIRE(b.flush());
    REQUIRE(b.size() == 0);
    REQUIRE(p.entry_count() == 2);
}

TEST_CASE_METHOD(BufferFixture, "Buffer flushes after timeout", "[buffer]") {
    TestClock::reset();

    minidb::Pager     p(TEST_FILE);
    minidb::Buffer<TestClock> b(p, minidb::Buffer<TestClock>::MIN_SIZE);

    (void)b.push(minidb::Entry{1704067200, 1});
    (void)b.push(minidb::Entry{1704067201, 2});

    TestClock::advance(std::chrono::seconds(minidb::Buffer<>::TIMEOUT+1)); //TIMEOUT + 1
    (void)b.push(minidb::Entry{1704067202, 3});  //Timeout

    REQUIRE(p.entry_count() >= 2);
}

TEST_CASE_METHOD(BufferFixture, "Buffer grows under high load", "[buffer]") {
    minidb::Pager    p(TEST_FILE);
    minidb::Buffer<> b(p, minidb::Buffer<>::MIN_SIZE);

    const std::size_t initial_capacity = b.capacity();

    for (int round = 0; round < 3; ++round)
        for (std::size_t i = 0; i < initial_capacity; ++i)
            (void)b.push(minidb::Entry{static_cast<uint32_t>(1704067200 + i), 1});

    REQUIRE(b.capacity() > initial_capacity);
}