#include "minidb/database.hpp"
#include <shared_mutex>
#include <mutex>

namespace minidb {

Database::Database(const std::filesystem::path& path, std::size_t buffer_size)
    : pager_(path)
    , buffer_(pager_, buffer_size)
{}

Database::~Database() {
    flush();
}

bool Database::insert(const Entry& e) {
    std::unique_lock lock(mutex_);
    return buffer_.push(e);
}

bool Database::flush() {
    std::unique_lock lock(mutex_);
    return buffer_.flush();
}

std::vector<Entry> Database::read_all_entries() const {
    const auto bytes = pager_.read_all();
    std::vector<Entry> entries;
    entries.reserve(bytes.size() / Entry::SIZE);

    for (std::size_t i = 0; i + Entry::SIZE <= bytes.size(); i += Entry::SIZE) {
        std::array<uint8_t, Entry::SIZE> raw{};
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i),
                    Entry::SIZE, raw.begin());

        if (auto e = Entry::deserialize(raw))
            entries.push_back(*e);
    }
    return entries;
}

std::vector<Entry> Database::query_all() const {
    std::shared_lock lock(mutex_);
    return read_all_entries();
}

std::vector<Entry> Database::query_by_device(uint8_t device_id) const {
    std::shared_lock lock(mutex_);
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
        return e.timestamp < from_ts || e.timestamp > to_ts;
    });
    return all;
}

} // namespace minidb