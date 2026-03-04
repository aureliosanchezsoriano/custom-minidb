#include "minidb/buffer.hpp"
#include <chrono>

namespace minidb {

using clock = std::chrono::steady_clock;

Buffer::Buffer(Pager& pager, std::size_t initial_size)
    : pager_(pager)
    , capacity_(std::clamp(initial_size, MIN_SIZE, MAX_SIZE))
    , last_flush_(clock::now())
{
    buffer_.reserve(capacity_);
}

bool Buffer::push(const Entry& e) {
    buffer_.push_back(e);

    if (buffer_.size() >= capacity_) {
        const auto now     = clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                 now - last_flush_).count();
        // Grew too fast: increase capacity
        if (elapsed < 2) grow();
        return flush();
    }

    // Buffer not full but has been idle too long: flush anyway
    const auto now     = clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                             now - last_flush_).count();
    if (elapsed > 30 && !buffer_.empty())
        return flush();

    return true;
}

bool Buffer::flush() {
    if (buffer_.empty()) return true;

    // Serialize all entries into a contiguous byte block
    std::vector<uint8_t> bytes;
    bytes.reserve(buffer_.size() * Entry::SIZE);

    for (const auto& e : buffer_) {
        const auto serialized = e.serialize();
        bytes.insert(bytes.end(), serialized.begin(), serialized.end());
    }

    const bool ok = pager_.write(bytes);
    if (ok) {
        // Check if buffer has been idle: shrink if so
        const auto now     = clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                 now - last_flush_).count();
        if (elapsed > 30) shrink();

        buffer_.clear();
        last_flush_ = clock::now();
    }
    return ok;
}

std::size_t Buffer::size()     const noexcept { return buffer_.size(); }
std::size_t Buffer::capacity() const noexcept { return capacity_; }

void Buffer::grow() {
    capacity_ = std::min(capacity_ * 2, MAX_SIZE);
    buffer_.reserve(capacity_);
}

void Buffer::shrink() {
    capacity_ = std::max(capacity_ / 2, MIN_SIZE);
}

} // namespace minidb