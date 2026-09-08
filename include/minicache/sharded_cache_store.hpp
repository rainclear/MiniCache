#ifndef MINICACHE_SHARDED_CACHE_STORE_HPP
#define MINICACHE_SHARDED_CACHE_STORE_HPP

#include "minicache/cache_store.hpp"
#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <functional>
#include <cstddef>
#include <cstdint>

namespace minicache {

/**
 * @brief Sharded In-Memory Cache Engine that partitions keys across multiple
 *        CacheStore shards to minimize lock contention under concurrent workloads.
 */
class ShardedCacheStore {
public:
    explicit ShardedCacheStore(std::size_t total_capacity = 100, std::size_t shard_count = 16)
        : shard_count_(shard_count > 0 ? shard_count : 16) {
        
        std::size_t capacity_per_shard = (total_capacity + shard_count_ - 1) / shard_count_;
        shards_.reserve(shard_count_);

        for (std::size_t i = 0; i < shard_count_; ++i) {
            shards_.push_back(std::make_unique<CacheStore>(capacity_per_shard));
        }
    }

    ~ShardedCacheStore() = default;

    // Non-copyable, move-only
    ShardedCacheStore(const ShardedCacheStore&) = delete;
    ShardedCacheStore& operator=(const ShardedCacheStore&) = delete;
    ShardedCacheStore(ShardedCacheStore&&) noexcept = default;
    ShardedCacheStore& operator=(ShardedCacheStore&&) noexcept = default;

    /**
     * @brief Route set operation to the corresponding shard.
     */
    void set(const std::string& key, const std::string& value, std::int64_t ttl_ms = -1) {
        get_shard(key).set(key, value, ttl_ms);
    }

    /**
     * @brief Route get operation to the corresponding shard.
     */
    [[nodiscard]] std::optional<std::string> get(const std::string& key) {
        return get_shard(key).get(key);
    }

    /**
     * @brief Route del operation to the corresponding shard.
     */
    bool del(const std::string& key) {
        return get_shard(key).del(key);
    }

    /**
     * @brief Returns total active elements aggregated across all shards.
     */
    [[nodiscard]] std::size_t size() const {
        std::size_t total_size = 0;
        for (const auto& shard : shards_) {
            total_size += shard->size();
        }
        return total_size;
    }

    [[nodiscard]] std::size_t shard_count() const noexcept {
        return shard_count_;
    }

private:
    [[nodiscard]] CacheStore& get_shard(const std::string& key) noexcept {
        std::size_t hash = hasher_(key);
        return *shards_[hash % shard_count_];
    }

    std::size_t shard_count_;
    std::vector<std::unique_ptr<CacheStore>> shards_;
    std::hash<std::string> hasher_;
};

} // namespace minicache

#endif // MINICACHE_SHARDED_CACHE_STORE_HPP