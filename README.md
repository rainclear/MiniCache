# MiniCache

**MiniCache** is a lightweight, high-performance, thread-safe embedded in-memory key-value storage engine built with **Modern C++20**. 

Designed as an in-process library (similar in architecture to SQLite or LevelDB), MiniCache provides microsecond-level latency, LRU eviction, TTL expiration support, and fine-grained concurrent access without networking or serialization overhead.

---

## Key Features

* **In-Process Embedded Architecture**: Zero network IPC overhead, directly compiled into your target application.
* **Modern C++20 Standard**: Leverages `std::optional` for explicit interface semantics, RAII memory bounds, and standard concurrency primitives.
* **$O(1)$ LRU Eviction**: Maintains strict access-order eviction using `std::unordered_map` coupled with `std::list::splice` node migration.
* **TTL Expiration**: Supports per-key time-to-live policies with lazy deletion on expired key lookups.
* **High-Concurrency Read/Write**: Implements a shared/exclusive locking model via `std::shared_mutex` (`std::shared_lock` for read, `std::unique_lock` for write).
* **CMake Build Automation**: Modular project layout supporting seamless cross-platform builds.

---

## Architecture & Data Flow