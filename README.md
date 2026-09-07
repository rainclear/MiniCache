# MiniCache

**MiniCache** is a high-performance, thread-safe embedded in-memory key-value storage engine built with **Modern C++20**. 

Designed as an in-process library (architecturally similar to LevelDB or SQLite), MiniCache offers microsecond-level access latency, zero-copy string parsing, $O(1)$ LRU cache eviction, TTL expiration policies, and an automated smart-pointer memory pool without networking overhead.

---

## 🌟 Key Features & Evolution Roadmap

### Phase 1: Core Embedded Storage Engine (`CacheStore`)
* **$O(1)$ LRU Eviction**: Doubly-linked list (`std::list`) paired with `std::unordered_map` and `std::list::splice` node relocation.
* **TTL Expiration**: Per-key Time-To-Live expiration with lazy deletion on access.
* **Modern Interface**: Leverages `std::optional<std::string>` for explicit, nullable return semantics.
* **High-Concurrency Model**: Multi-reader single-writer locking model via `std::shared_mutex` (`std::shared_lock` vs `std::unique_lock`).

### Phase 2: Command Pattern & Text Parser Engine (`CommandFactory`)
* **Extensible Command Hierarchy**: Abstract `Command` base class driving concrete `SetCommand`, `GetCommand`, and `DelCommand` implementations.
* **Factory Pattern with Self-Registration**: Dynamic command parsing utilizing `std::unordered_map` and Lambda handlers.
* **Protocol-Agnostic Interface**: Supports raw text instruction tokenization (with string quote handling), enabling future RESP / REST protocol wrappers or CLI debugging shells.

### Phase 3: Generic Object Pool (`ObjectPool`)
* **Zero-Allocation Memory Reuse**: Thread-safe memory pool reducing heap fragmentation for heavy buffers and temporary objects.
* **RAII Auto-Recycling**: Uses `std::unique_ptr<T, CustomDeleter>` with a weak reference back to the pool, automatically recycling objects when leaving scope.
* **C++20 Concept Integration**: Enforces state cleanup via `Resettable<T>` concept hooks (`ptr->reset()`) prior to object recycling.

---

## 📐 Architecture & Layering Model

```text
+-------------------------------------------------------------------+
|               Client Application / Upper Interface                |
|       (Direct C++ Native API / CLI Shell / Protocol Layer)        |
+-------------------------------------------------------------------+
                                  |
            [Text Commands]       |       [Direct C++ Calls]
                   |              |               |
                   v              |               |
+---------------------------------+---------------+-----------------+
|   Command Layer (Phase 2)       |               |                 |
|   - CommandFactory              |               |                 |
|   - Command Interface           |               |                 |
|     (SET / GET / DEL)           |               |                 |
+---------------------------------+               |                 |
                   |                              |                 |
                   +------------------------------+                 |
                                  |                                 |
                                  v                                 v
+-------------------------------------------------------------------+
|   Core Storage Engine (Phase 1)       Memory Layer (Phase 3)      |
|   - CacheStore (LRU & TTL)            - ObjectPool<T>             |
|   - std::shared_mutex                 - Custom Deleter RAII       |
|   - std::unordered_map + std::list    - C++20 Resettable Concept  |
+-------------------------------------------------------------------+