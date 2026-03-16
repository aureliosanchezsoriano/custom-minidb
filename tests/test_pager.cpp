#include "../vendor/catch2/catch.hpp"

#include "minidb/pager.hpp"
#include "minidb/entry.hpp"

#include <filesystem>

static const std::filesystem::path TEST_FILE = "test_pager.bin";

struct PagerFixture {
    PagerFixture()  { std::filesystem::remove(TEST_FILE); }
    ~PagerFixture() { std::filesystem::remove(TEST_FILE); }
};

TEST_CASE_METHOD(PagerFixture, "Pager writes and reads back", "[pager]") {
    minidb::Entry e{1704067200, 1};
    auto bytes = e.serialize();

    {
        minidb::Pager p(TEST_FILE);
        REQUIRE(p.write(bytes));
    }

    minidb::Pager p(TEST_FILE);
    auto data = p.read_all();

    REQUIRE(data.size() == minidb::Entry::SIZE);
    REQUIRE(data == std::vector<uint8_t>(bytes.begin(), bytes.end()));
}

TEST_CASE_METHOD(PagerFixture, "Pager entry_count is correct", "[pager]") {
    minidb::Pager p(TEST_FILE);

    minidb::Entry e1{1704067200, 1};
    minidb::Entry e2{1704067201, 2};

    (void)p.write(e1.serialize());
    (void)p.write(e2.serialize());

    REQUIRE(p.entry_count() == 2);
}

TEST_CASE_METHOD(PagerFixture, "Pager appends correctly", "[pager]") {
    minidb::Entry e1{1704067200, 1};
    minidb::Entry e2{1704067201, 2};

    {
        minidb::Pager p(TEST_FILE);
        (void)p.write(e1.serialize());
    }
    {
        minidb::Pager p(TEST_FILE);
        (void)p.write(e2.serialize());
    }

    minidb::Pager p(TEST_FILE);
    REQUIRE(p.entry_count() == 2);
}