#include "../vendor/catch2/catch.hpp"

#include "minidb/database.hpp"

#include <filesystem>


static const std::filesystem::path TEST_FILE = "test_database.bin";

struct DatabaseFixture {
    DatabaseFixture()  { std::filesystem::remove(TEST_FILE); }
    ~DatabaseFixture() { std::filesystem::remove(TEST_FILE); }
};

TEST_CASE_METHOD(DatabaseFixture, "Database insert and query_all", "[database]") {
    minidb::Database db(TEST_FILE);

    (void)db.insert(minidb::Entry{1704067200, 1});
    (void)db.insert(minidb::Entry{1704067201, 2});
    (void)db.insert(minidb::Entry{1704067202, 3});
    (void)db.flush();

    auto all = db.query_all();
    REQUIRE(all.size() == 3);
}

TEST_CASE_METHOD(DatabaseFixture, "Database query_by_device", "[database]") {
    minidb::Database db(TEST_FILE);

    (void)db.insert(minidb::Entry{1704067200, 1});
    (void)db.insert(minidb::Entry{1704067201, 2});
    (void)db.insert(minidb::Entry{1704067202, 1});
    (void)db.flush();

    auto results = db.query_by_device(1);
    REQUIRE(results.size() == 2);
    for (const auto& e : results)
        REQUIRE(e.device_id == 1);
}

TEST_CASE_METHOD(DatabaseFixture, "Database query_by_range", "[database]") {
    minidb::Database db(TEST_FILE);

    (void)db.insert(minidb::Entry{1704067200, 1});
    (void)db.insert(minidb::Entry{1704067210, 2});
    (void)db.insert(minidb::Entry{1704067220, 3});
    (void)db.flush();

    auto results = db.query_by_range(1704067205, 1704067215);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].device_id == 2);
}

TEST_CASE_METHOD(DatabaseFixture, "Database persists across instances", "[database]") {
    {
        minidb::Database db(TEST_FILE);
        (void)db.insert(minidb::Entry{1704067200, 1});
        (void)db.insert(minidb::Entry{1704067201, 2});
    }

    minidb::Database db(TEST_FILE);
    auto all = db.query_all();
    REQUIRE(all.size() == 2);
}