#pragma once

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <vector>
#include <fstream>
#include <span>

namespace minidb {

class Pager {
public:
    explicit Pager(const std::filesystem::path& path);
    ~Pager();

  // Write a block of data to the file. Returns true on success, false on
  // failure.
  [[nodiscard]] bool write(const std::span<const uint8_t> data);

  // Read all data from the file
  [[nodiscard]] std::vector<uint8_t> read_all() const;

  // Get the number of entries in the file
  [[nodiscard]] size_t entry_count();

  Pager(const Pager &) = delete;
  Pager &operator=(const Pager &) = delete;
private:
  std::filesystem::path path_;
  std::fstream file_;
};
}