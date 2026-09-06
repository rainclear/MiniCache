#ifndef MINICACHE_CACHE_STORE_HPP
#define MINICACHE_CACHE_STORE_HPP

#include <string>
#include <unordered_map>
#include <list>
#include <optional>
#include <chrono>
#include <shared_mutex>
#include <cstdint>

namespace minicache {

/**
 * @brief Represents a single cached entity with expiration tracking.
 */
struct CacheNode {
    std::string key;
    std::string value;
    std::chrono::steady_clock::time_point expire_at;

    [[nodiscard]] bool is_expired() const noexcept {
        return std::chrono::steady_clock::now() > expire_at;
    }
};

/**
 * @brief Thread-safe In-Memory KV Cache Engine supporting LRU Eviction & TTL.
 */
class CacheStore {
public:
    using NodeList = std::list<CacheNode>;
    using NodeIter = NodeList::iterator;

    explicit CacheStore(std::size_t capacity = 100);
    ~CacheStore() = default;

    // Non-copyable, move-only
    CacheStore(const CacheStore&) = delete;
    CacheStore& operator=(const CacheStore&) = delete;
    CacheStore(CacheStore&&) noexcept = default;
    CacheStore& operator=(CacheStore&&) noexcept = default;

    /**
     * @brief Inserts or updates a key-value pair with an optional TTL in milliseconds.
     */
    void set(const std::string& key, const std::string& value, std::int64_t ttl_ms = -1);

    /**
     * @brief Retrieves a value associated with the given key.
     * @return std::optional containing the value if found and not expired; std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<std::string> get(const std::string& key);

    /**
     * @brief Removes a key-value pair from the cache.
     * @return true if the key was present and removed; false otherwise.
     */
    bool del(const std::string& key);

    /**
     * @brief Returns the current number of active elements in the cache.
     */
    [[nodiscard]] std::size_t size() const;

private:
    void evict();

    std::size_t capacity_;
    NodeList lru_list_;
    std::unordered_map<std::string, NodeIter> map_;
    mutable std::shared_mutex mutex_;
};

} // namespace minicache

#endif // MINICACHE_CACHE_STORE_HPP