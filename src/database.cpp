#include "minidb/database.hpp"
#include <shared_mutex>
#include <mutex>

namespace minidb {

Database::Database(const std::filesystem::path& path, std::size_t buffer_size)
    : pager_(path)
    , buffer_(pager_, buffer_size)
{}
// Destructor flushes any remaining entries in the buffer to disk.
Database::~Database() {
    flush();
}
// Inserts a new entry into the database. Returns true on success, false on failure.
bool Database::insert(const Entry& e) {
    std::unique_lock lock(mutex_); // Ensure exclusive access for writes
    return buffer_.push(e);
}
// Forces a flush of the buffer to disk. Returns true on success, false on failure.
bool Database::flush() {
    std::unique_lock lock(mutex_); // Ensure exclusive access for flush
    return buffer_.flush();
}
// Returns all entries in the database.
std::vector<Entry> Database::read_all_entries() const {
    // Read all bytes from the pager and deserialize into entries, skipping any corrupted records (invalid CRC).
    const auto bytes = pager_.read_all();
    std::vector<Entry> entries;
    entries.reserve(bytes.size() / Entry::SIZE);

    // Deserialize bytes into entries, skipping any corrupted records (invalid CRC)
    for (std::size_t i = 0; i + Entry::SIZE <= bytes.size(); i += Entry::SIZE) {
        std::array<uint8_t, Entry::SIZE> raw{};
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i),
                    Entry::SIZE, raw.begin());
        // If deserialization fails (invalid CRC), skip the record
        if (auto e = Entry::deserialize(raw))
            entries.push_back(*e);
    }
    return entries;
}
// Returns all entries in the database.
std::vector<Entry> Database::query_all() const {
    std::shared_lock lock(mutex_); // Allow concurrent reads
    return read_all_entries();
}
// Returns entries matching a device_id.
std::vector<Entry> Database::query_by_device(uint8_t device_id) const {
    std::shared_lock lock(mutex_); // Allow concurrent reads
    auto all = read_all_entries();
    std::erase_if(all, [device_id](const Entry& e) {
        return e.device_id != device_id;
    });
    return all;
}

std::vector<Entry> Database::query_by_range(uint32_t from_ts, uint32_t to_ts) const {
    std::shared_lock lock(mutex_);
    auto all = read_all_entries();
    std::erase_if(all, [from_ts, to_ts](const Entry& e) {
        return e.timestamp < from_ts || e.timestamp > to_ts; // Keep only entries within the specified timestamp range
    });
    return all;
}

} // namespace minidb