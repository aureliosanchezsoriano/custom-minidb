# custom-minidb

A lightweight, high-performance binary database engine written in modern C++20,
designed for resource-constrained environments with high write frequency and
low read frequency. Built as the persistence layer for an ESP32/MQTT presence
detection system running on Raspberry Pi.

---

## Features

- **Compact binary format** — 6 bytes per entry (4B timestamp + 1B device ID + 1B CRC-8)
- **CRC-8/SMBUS integrity checking** — detects corruption on every read
- **Dynamic write buffer** — automatically adjusts size based on write frequency
- **Append-only writes** — O(1) insertions, safe against power loss
- **Little-endian serialization** — portable across ARM and x86 architectures
- **Modern C++20** — `std::optional`, `std::span`, `std::chrono`, `constexpr` throughout
- **Header-only buffer** — zero-overhead template-based timeout configuration
- **Thread-safe** — readers-writer lock via `std::shared_mutex`
---

## Architecture

The library is built in three layers, each with a single responsibility:
```
Database          → public API: insert(), query_all(), query_by_device(), query_by_range()
    │
Buffer<T>         → dynamic write buffer, flush logic, timeout management
    │
Pager             → binary file I/O, append and read operations
    │
Entry             → data struct, serialization, CRC-8 integrity
```
Thread safety is handled via `std::shared_mutex`:

- **Writes** (`insert`, `flush`) acquire an exclusive `std::unique_lock` — no reads or writes can happen concurrently
- **Reads** (`query_*`) acquire a shared `std::shared_lock` — multiple concurrent reads are allowed, but writes are blocked

Write operations are queued and never dropped.

### Entry

The smallest unit of data. Represents a single presence detection event.

On-disk layout (6 bytes, little-endian):
```
Byte  0            3  4         5
      ┌────────────┬──────────┬──────────┐
      │ timestamp  │ device_id│   crc8   │
      │  4 bytes   │  1 byte  │  1 byte  │
      └────────────┴──────────┴──────────┘
```

- **timestamp**: Unix timestamp in seconds, valid until year 2106
- **device_id**: device identifier, 1–255 (0 is reserved)
- **crc8**: CRC-8/SMBUS checksum of the first 5 bytes, used to detect corruption

### Pager

Handles all binary file I/O. Opens the file in append mode and keeps it open
for the lifetime of the object. Writes are always appended to the end of the
file. Reads load the entire file into memory.

### Buffer

Accumulates entries in RAM and flushes them to the Pager in batches. This
minimizes syscall overhead for high-frequency writes.

The buffer is implemented as a C++20 template parameterized by timeout:
```cpp
template<std::size_t TimeoutSeconds = 30>
class Buffer;
```

**Dynamic sizing** — the buffer adapts its capacity based on write frequency:

- If the buffer fills up in less than 2 seconds, capacity doubles (up to 1024)
- If the buffer has been idle for longer than the timeout, capacity halves (down to 16)

**Two flush triggers:**

| Trigger | Condition |
|---|---|
| Size-based | Buffer reaches capacity |
| Time-based | No flush for `TimeoutSeconds` seconds |

The destructor of `Database` always flushes the buffer, so data is never lost
on clean shutdown.

### Database

The public API. Owns a `Pager` and a `Buffer`, exposes insert and query
operations. Queries load all entries from disk and filter in memory, which is
efficient for the expected data volume (a few MB per year).

---

## On-disk format

Records are stored sequentially with no header, no separators, and no metadata:
```
[Entry 0][Entry 1][Entry 2]...
 6 bytes   6 bytes   6 bytes
```

The total number of entries is always `file_size / 6`. The position of entry N
is always `N * 6`. No parsing required.

---

## Requirements

- C++20 compiler (GCC 10+, Clang 12+)
- CMake 3.20+

---

## Building
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## Running tests
```bash
cd build
ctest --verbose
```

Tests use [Catch2 v2.13.10](https://github.com/catchorg/Catch2) (included in `vendor/`).

---

## Usage

### Basic example
```cpp
#include <minidb/database.hpp>
#include <chrono>

int main() {
    minidb::Database db("detections.bin");

    // Get current Unix timestamp
    uint32_t ts = static_cast<uint32_t>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );

    // Insert a detection event
    db.insert(minidb::Entry{ts, 1});  // device 1

    // Query all entries
    auto all = db.query_all();

    // Query by device
    auto from_device_2 = db.query_by_device(2);

    // Query by time range
    auto in_range = db.query_by_range(1704067200, 1704067300);

    return 0;  // destructor flushes buffer automatically
} 
```

### Using as a git submodule
```bash
git submodule add https://github.com/yourusername/custom-minidb libs/custom-minidb
```

In your `CMakeLists.txt`:
```cmake
add_subdirectory(libs/custom-minidb)
target_link_libraries(your-target PRIVATE minidb)
```

When cloning a project that uses custom-minidb as a submodule:
```bash
git clone --recurse-submodules https://github.com/yourusername/your-project
```

### Configuring the buffer timeout

By default the buffer flushes after 30 seconds of inactivity. This is
configured at compile time via the template parameter:
```cpp
// Production: flush after 30 seconds (default)
minidb::Buffer<> buffer;

// Custom timeout: flush after 5 seconds
minidb::Buffer<5> buffer;
```

The `Database` class uses `Buffer<>` by default. If you need a custom timeout,
instantiate `Buffer` and `Pager` directly.

---

## Storage estimates

At 100 detections/hour across all devices:

| Period | Entries | Storage |
|---|---|---|
| 1 day | 2,400 | ~14 KB |
| 1 month | 72,000 | ~422 KB |
| 1 year | 876,000 | ~5 MB |

---

## License

MIT