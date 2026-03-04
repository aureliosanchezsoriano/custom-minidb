#include "../vendor/catch2/catch.hpp"
#include "minidb/pager.hpp"
#include <filesystem>

using namespace minidb;

static const std::filesystem::path TEST_FILE = "test_pager.bin";

struct PagerFixture {
    PagerFixture()  { std::filesystem::remove(TEST_FILE); }
    ~PagerFixture() { std::filesystem::remove(TEST_FILE); }
};

TEST_CASE_METHOD(PagerFixture, "Pager writes and reads back", "[pager]") {
    Entry e{1704067200, 1};
    auto bytes = e.serialize();

    {
        Pager p(TEST_FILE);
        REQUIRE(p.write(bytes));
    }

    Pager p(TEST_FILE);
    auto data = p.read_all();

    REQUIRE(data.size() == Entry::SIZE);
    REQUIRE(data == std::vector<uint8_t>(bytes.begin(), bytes.end()));
}

TEST_CASE_METHOD(PagerFixture, "Pager entry_count is correct", "[pager]") {
    Pager p(TEST_FILE);

    Entry e1{1704067200, 1};
    Entry e2{1704067201, 2};

    p.write(e1.serialize());
    p.write(e2.serialize());

    REQUIRE(p.entry_count() == 2);
}

TEST_CASE_METHOD(PagerFixture, "Pager appends correctly", "[pager]") {
    Entry e1{1704067200, 1};
    Entry e2{1704067201, 2};

    {
        Pager p(TEST_FILE);
        p.write(e1.serialize());
    }
    {
        Pager p(TEST_FILE);
        p.write(e2.serialize());
    }

    Pager p(TEST_FILE);
    REQUIRE(p.entry_count() == 2);
}