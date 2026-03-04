# custom-minidb

A lightweight, high-performance binary database engine written in modern C++20. Designed to store sensor event logs in resource-constrained environments.

## Overview

custom-minidb is a minimal embedded database built from scratch with a focus on efficiency. It stores data in a compact binary format with CRC-8 integrity checking, making it ideal for IoT and embedded systems where storage and performance matter.

## Features

- **Compact binary format** — 6 bytes per entry (4 bytes timestamp + 1 byte device ID + 1 byte CRC-8)
- **CRC-8/SMBUS integrity checking** — detects corruption on every read
- **Dynamic write buffer** — automatically adjusts buffer size based on write frequency
- **Append-only writes** — O(1) insertions, safe against power loss
- **Little-endian serialization** — portable across ARM and x86 architectures
- **Modern C++20** — uses `std::optional`, `std::span`, `std::chrono` and `constexpr` throughout

## Architecture
```
database.hpp/cpp     → public API: insert(), query()
      │
buffer.hpp/cpp       → dynamic write buffer, flush logic
      │
pager.hpp/cpp        → binary file I/O
      │
record.hpp           → entry struct, serialization, CRC-8
```

## On-disk format

Each entry occupies exactly 6 bytes:
```
Offset 0           3 4         5
       ┌────────────┬──────────┬──────────┐
       │ timestamp  │ device_id│   crc8   │
       │  4 bytes   │  1 byte  │  1 byte  │
       └────────────┴──────────┴──────────┘
```

- **timestamp**: Unix timestamp in seconds, valid until 2106
- **device_id**: device identifier, 1–255 (0 is reserved)
- **crc8**: CRC-8/SMBUS checksum of the first 5 bytes