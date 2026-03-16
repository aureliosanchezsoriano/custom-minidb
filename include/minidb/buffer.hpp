#pragma once

#include "entry.hpp"
#include "pager.hpp"

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <chrono>


namespace minidb {

// Buffer class that batches entries in memory before writing to disk.
template<typename Clock = std::chrono::steady_clock>
class Buffer {
public:
    static constexpr std::size_t MIN_SIZE     = 16;
    static constexpr std::size_t MAX_SIZE     = 1024;
    static constexpr std::size_t DEFAULT_SIZE = 64;
    static constexpr std::size_t TIMEOUT      = 30;

    explicit Buffer(minidb::Pager& pager, std::size_t initial_size = DEFAULT_SIZE)
        : pager_(pager)
        , capacity_(std::clamp(initial_size, MIN_SIZE, MAX_SIZE))
        , last_flush_(Clock::now())
    {
        buffer_.reserve(capacity_);
    }

    [[nodiscard]] bool push(const minidb::Entry& e) {
        buffer_.push_back(e);

        // If we've reached capacity, flush immediately. If we flushed too recently, grow the buffer to reduce flush frequency.
        if (buffer_.size() >= capacity_) {
            // Check how long it's been since the last flush
            const auto now     = Clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                     now - last_flush_).count();
            const bool ok = flush();
            // If we flushed too quickly, grow the buffer to reduce flush frequency
            if (elapsed < 2) grow();
            return ok;
        }
        // Check if we should flush based on time since last flush
        const auto now     = Clock::now();
        const std::size_t elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                 now - last_flush_).count();
        if (elapsed > TIMEOUT && !buffer_.empty())
            return flush();

        return true;
    }

    [[nodiscard]] bool flush() {
        if (buffer_.empty()) return true;

        std::vector<uint8_t> bytes;
        bytes.reserve(buffer_.size() * Entry::SIZE);

        // Serialize all entries in the buffer
        for (const auto& e : buffer_) {
            const auto serialized = e.serialize();
            bytes.insert(bytes.end(), serialized.begin(), serialized.end());
        }

        const bool ok = pager_.write(bytes); // Write to disk
        if (ok) {
            // Check if we should grow or shrink the buffer based on time since last flush
            const auto now     = Clock::now();
            const std::size_t elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                     now - last_flush_).count();
            // If we flushed too quickly, grow the buffer to reduce flush frequency
            if (elapsed > TIMEOUT) shrink();
            buffer_.clear();
            last_flush_ = Clock::now();
        }
        return ok;
    }

    [[nodiscard]] std::size_t size()     const noexcept { return buffer_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;

private:
    Pager&             pager_;
    std::vector<Entry> buffer_;
    std::size_t        capacity_;
    Clock::time_point last_flush_;

    void grow() {
        capacity_ = std::min(capacity_ * 2, MAX_SIZE);
        buffer_.reserve(capacity_);
    }

    void shrink() {
        capacity_ = std::max(capacity_ / 2, MIN_SIZE);
    }
};

} // namespace minidb