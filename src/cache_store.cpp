#include "minicache/cache_store.hpp"
#include <mutex>

namespace minicache {

CacheStore::CacheStore(std::size_t capacity) : capacity_(capacity) {}

void CacheStore::set(const std::string& key, const std::string& value, std::int64_t ttl_ms) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    // Calculate expiration timestamp
    const auto expire_time = (ttl_ms > 0)
        ? std::chrono::steady_clock::now() + std::chrono::milliseconds(ttl_ms)
        : std::chrono::steady_clock::time_point::max();

    auto it = map_.find(key);
    if (it != map_.end()) {
        // Key exists: Update value and expiration, move node to LRU head
        NodeIter node_it = it->second;
        node_it->value = value;
        node_it->expire_at = expire_time;
        lru_list_.splice(lru_list_.begin(), lru_list_, node_it);
        return;
    }

    // Evict least recently used item if capacity limit reached
    if (lru_list_.size() >= capacity_) {
        evict();
    }

    // Insert new item to LRU head
    lru_list_.push_front(CacheNode{key, value, expire_time});
    map_[key] = lru_list_.begin();
}

std::optional<std::string> CacheStore::get(const std::string& key) {
    // Phase 1: Try reading under a shared lock
    {
        std::shared_lock<std::shared_mutex> read_lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }

        if (!it->second->is_expired()) {
            // Upgrade lock to promote node to LRU head (C++20 double-check pattern)
            read_lock.unlock();
            
            std::unique_lock<std::shared_mutex> write_lock(mutex_);
            auto write_it = map_.find(key);
            if (write_it != map_.end() && !write_it->second->is_expired()) {
                lru_list_.splice(lru_list_.begin(), lru_list_, write_it->second);
                return write_it->second->value;
            }
            return std::nullopt;
        }
    }

    // Phase 2: Lazy deletion for expired items under unique lock
    std::unique_lock<std::shared_mutex> write_lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end() && it->second->is_expired()) {
        lru_list_.erase(it->second);
        map_.erase(it);
    }
    return std::nullopt;
}

bool CacheStore::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end()) {
        return false;
    }

    lru_list_.erase(it->second);
    map_.erase(it);
    return true;
}

std::size_t CacheStore::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return map_.size();
}

void CacheStore::evict() {
    if (lru_list_.empty()) {
        return;
    }
    // Remove the tail item (least recently used)
    auto last_it = std::prev(lru_list_.end());
    map_.erase(last_it->key);
    lru_list_.pop_back();
}

} // namespace minicache