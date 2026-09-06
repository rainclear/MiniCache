#include "minicache/cache_store.hpp"
#include <iostream>
#include <cassert>
#include <thread>

int main() {
    std::cout << "=== MiniCache C++20 Test Engine ===\n\n";

    // Initialize Cache with capacity = 2 for LRU testing
    minicache::CacheStore cache(2);

    // 1. Basic SET and GET
    cache.set("k1", "v1");
    cache.set("k2", "v2");

    auto val1 = cache.get("k1");
    assert(val1.has_value());
    std::cout << "[PASS] GET k1: " << val1.value_or("N/A") << '\n';

    // 2. LRU Eviction: k1 accessed, k2 is now LRU. Inserting k3 should evict k2.
    cache.set("k3", "v3");

    auto val2 = cache.get("k2");
    assert(!val2.has_value());
    std::cout << "[PASS] GET k2 (Evicted): " 
              << (val2 ? *val2 : "NULL") << '\n';

    auto val3 = cache.get("k3");
    assert(val3.has_value() && *val3 == "v3");
    std::cout << "[PASS] GET k3: " << *val3 << '\n';

    // 3. TTL Expiration
    std::cout << "\nSetting 'k4' with 100ms TTL...\n";
    cache.set("k4", "v4_temp", 100);

    auto val4_immediate = cache.get("k4");
    assert(val4_immediate.has_value());
    std::cout << "[PASS] GET k4 (Immediate): " << *val4_immediate << '\n';

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    auto val4_expired = cache.get("k4");
    assert(!val4_expired.has_value());
    std::cout << "[PASS] GET k4 (Expired): NULL\n";

    std::cout << "\n=== All Phase 1 Tests Passed Successfully! ===\n";
    return 0;
}