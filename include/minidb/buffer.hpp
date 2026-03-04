#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <chrono>
#include "minidb/entry.hpp"
#include "minidb/pager.hpp"

namespace minidb {

class Buffer {
public:
    static constexpr std::size_t MIN_SIZE     = 16;
    static constexpr std::size_t MAX_SIZE     = 1024;
    static constexpr std::size_t DEFAULT_SIZE = 64;

    explicit Buffer(Pager& pager, std::size_t initial_size = DEFAULT_SIZE);

    // Adds an entry to the buffer, flushes if full
    [[nodiscard]] bool push(const Entry& e);

    // Forces a flush of all buffered entries to disk
    [[nodiscard]] bool flush();

    // Returns the number of entries currently in the buffer
    [[nodiscard]] std::size_t size()     const noexcept;

    // Returns the current buffer capacity
    [[nodiscard]] std::size_t capacity() const noexcept;

private:
    Pager&             pager_;
    std::vector<Entry> buffer_;
    std::size_t        capacity_;

    // Dynamic sizing
    std::chrono::steady_clock::time_point last_flush_;
    void grow();
    void shrink();
};

} // namespace minidb