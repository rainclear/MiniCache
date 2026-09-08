#include "minicache/cache_store.hpp"
#include <mutex>
#include <thread>

namespace minicache {

CacheStore::CacheStore(std::size_t capacity, std::int64_t cleanup_interval_ms) 
    : capacity_(capacity) {
    if (cleanup_interval_ms > 0) {
        purge_thread_ = std::jthread([this, cleanup_interval_ms](std::stop_token stop_tok) {
            active_purge_loop(stop_tok, std::chrono::milliseconds(cleanup_interval_ms));
        });
    }
}

CacheStore::~CacheStore() {
    // std::jthread automatically signals stop_token and joins upon destruction!
}

void CacheStore::active_purge_loop(std::stop_token stop_tok, std::chrono::milliseconds interval) {
    while (!stop_tok.stop_requested()) {
        // Sleep in small increments or interruptible sleep
        std::this_thread::sleep_for(interval);

        if (stop_tok.stop_requested()) break;

        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (lru_list_.empty()) continue;

        // Iterate through LRU list and purge expired keys
        auto it = lru_list_.begin();
        while (it != lru_list_.end()) {
            if (it->is_expired()) {
                map_.erase(it->key);
                it = lru_list_.erase(it); // Returns iterator to next element
            } else {
                ++it;
            }
        }
    }
}

void CacheStore::set(const std::string& key, const std::string& value, std::int64_t ttl_ms) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    const auto expire_time = (ttl_ms > 0)
        ? std::chrono::steady_clock::now() + std::chrono::milliseconds(ttl_ms)
        : std::chrono::steady_clock::time_point::max();

    auto it = map_.find(key);
    if (it != map_.end()) {
        NodeIter node_it = it->second;
        node_it->value = value;
        node_it->expire_at = expire_time;
        lru_list_.splice(lru_list_.begin(), lru_list_, node_it);
        return;
    }

    if (lru_list_.size() >= capacity_) {
        evict();
    }

    lru_list_.push_front(CacheNode{key, value, expire_time});
    map_[key] = lru_list_.begin();
}

std::optional<std::string> CacheStore::get(const std::string& key) {
    {
        std::shared_lock<std::shared_mutex> read_lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }

        if (!it->second->is_expired()) {
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
    auto last_it = std::prev(lru_list_.end());
    map_.erase(last_it->key);
    lru_list_.pop_back();
}

} // namespace minicache