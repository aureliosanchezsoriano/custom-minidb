#pragma once

#include "minidb/buffer.hpp"
#include "minidb/entry.hpp"
#include "minidb/pager.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>
#include <shared_mutex>

namespace minidb {

class Database {
public:
  explicit Database(const std::filesystem::path &path,
                    std::size_t buffer_size = Buffer<>::DEFAULT_SIZE);
  ~Database();

  // Inserts a new entry
  [[nodiscard]] bool insert(const Entry &e);

  // Returns all entries
  [[nodiscard]] std::vector<Entry> query_all() const;

  // Returns entries matching a device_id
  [[nodiscard]] std::vector<Entry> query_by_device(uint8_t device_id) const;

  // Returns entries within a timestamp range
  [[nodiscard]] std::vector<Entry> query_by_range(uint32_t from_ts,
                                                  uint32_t to_ts) const;

  // Forces a flush of the buffer to disk
  [[nodiscard]] bool flush();

  // Non-copyable, non-movable
  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;

private:
  Pager pager_; // Manages file I/O
  Buffer<> buffer_; // In-memory buffer for batching writes
  mutable std::shared_mutex mutex_; // Protects concurrent access to the database

  [[nodiscard]] std::vector<Entry> read_all_entries() const;
};

} // namespace minidb