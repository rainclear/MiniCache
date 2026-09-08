#include "minicache/cache_store.hpp"
#include <iostream>
#include <cassert>
#include <thread>

int main() {
    std::cout << "=== MiniCache Active TTL Purge Test ===\n\n";

    // Create Cache with 50ms active cleanup interval
    minicache::CacheStore cache(10, 50);

    cache.set("perm_key", "value1");
    cache.set("temp_key", "value2", 100); // Expires in 100ms

    assert(cache.size() == 2);
    std::cout << "[PASS] Initial cache size: " << cache.size() << '\n';

    std::cout << "Sleeping 250ms (waiting for background jthread purge)...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Notice we do NOT call get("temp_key") - background thread should have purged it!
    assert(cache.size() == 1);
    std::cout << "[PASS] Cache size after active purge: " << cache.size() << '\n';

    auto perm = cache.get("perm_key");
    assert(perm.has_value() && *perm == "value1");

    std::cout << "\n=== Active TTL Purge Test Passed! ===\n";
    return 0;
}