#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace minidb {
// CRC-8/SMBUS polynomial: x^8 + x^2 + x + 1 (0x07)
// Detects single-bit errors and some multi-bit errors in the data.
[[nodiscard]] constexpr uint8_t crc8(const uint8_t *data,
                                     std::size_t len) noexcept {
  uint8_t crc = 0x00;
  for (std::size_t i = 0; i < len; ++i) {
    crc ^= data[i];                   // XOR current byte into CRC
    for (int bit = 0; bit < 8; ++bit) // process each bit
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
  }
  return crc;
}

// Database register serialized:
// On-disk layout of a entry (6 bytes, little-endian):
// timestamp(4 bytes) | device_id (1 byte) | crc8 (1 byte)
struct Entry {
  uint32_t timestamp; // Unix timestamp (seconds since Jan 1, 1970)
  uint8_t device_id;  // Device ID (0-255)

  // Byte offsets within the serialized record
  static constexpr std::size_t SIZE = 6;
  static constexpr std::size_t TIMESTAMP_OFFSET = 0;
  static constexpr std::size_t DEVICE_ID_OFFSET = 4;
  static constexpr std::size_t CRC_OFFSET = 5;

  // Serialize the record into 6 bytes (little-endian)
  // The CRC is computed over the first 5 bytes.
  [[nodiscard]] std::array<uint8_t, SIZE> serialize() const noexcept {
    std::array<uint8_t, SIZE> bytes{};

    bytes[TIMESTAMP_OFFSET + 0] = static_cast<uint8_t>((timestamp >> 0) & 0xFF);
    bytes[TIMESTAMP_OFFSET + 1] = static_cast<uint8_t>((timestamp >> 8) & 0xFF);
    bytes[TIMESTAMP_OFFSET + 2] =
        static_cast<uint8_t>((timestamp >> 16) & 0xFF);
    bytes[TIMESTAMP_OFFSET + 3] =
        static_cast<uint8_t>((timestamp >> 24) & 0xFF);
    bytes[DEVICE_ID_OFFSET] = device_id;
    bytes[CRC_OFFSET] = crc8(bytes.data(), SIZE - 1);

    return bytes;
  }

  // Deserialize 6 bytes into a record.
  // Returns nullopt if the CRC does not match (corrupted data).
  [[nodiscard]] static std::optional<Entry>
  deserialize(const std::array<uint8_t, SIZE> &bytes) noexcept {
    const uint8_t expected_crc = crc8(bytes.data(), SIZE - 1);
    if (bytes[CRC_OFFSET] != expected_crc)
      return std::nullopt;

    const uint32_t ts =
        (static_cast<uint32_t>(bytes[TIMESTAMP_OFFSET + 0]) << 0) |
        (static_cast<uint32_t>(bytes[TIMESTAMP_OFFSET + 1]) << 8) |
        (static_cast<uint32_t>(bytes[TIMESTAMP_OFFSET + 2]) << 16) |
        (static_cast<uint32_t>(bytes[TIMESTAMP_OFFSET + 3]) << 24);

    return Entry{ts, bytes[DEVICE_ID_OFFSET]};
  }
};
} // namespace minidb
