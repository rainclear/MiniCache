#ifndef MINICACHE_CACHE_STORE_HPP
#define MINICACHE_CACHE_STORE_HPP

#include <string>
#include <unordered_map>
#include <list>
#include <optional>
#include <chrono>
#include <shared_mutex>
#include <cstdint>
#include <thread>     // For std::jthread
#include <stop_token> // For std::stop_token

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

    /**
     * @param capacity Maximum number of items in cache.
     * @param cleanup_interval_ms Active purge interval in milliseconds. Set to 0 to disable.
     */
    explicit CacheStore(std::size_t capacity = 100, std::int64_t cleanup_interval_ms = 500);
    ~CacheStore();

    // Non-copyable, non-movable due to thread and mutex management
    CacheStore(const CacheStore&) = delete;
    CacheStore& operator=(const CacheStore&) = delete;
    CacheStore(CacheStore&&) = delete;
    CacheStore& operator=(CacheStore&&) = delete;

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
    void active_purge_loop(std::stop_token stop_tok, std::chrono::milliseconds interval);

    std::size_t capacity_;
    NodeList lru_list_;
    std::unordered_map<std::string, NodeIter> map_;
    mutable std::shared_mutex mutex_;

    // C++20 background worker thread
    std::jthread purge_thread_;
};

} // namespace minicache

#endif // MINICACHE_CACHE_STORE_HPP