#include "minicache/object_pool.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstring>

class NetworkBuffer {
public:
    static constexpr std::size_t kCapacity = 1024;

    NetworkBuffer() {
        std::memset(data_, 0, kCapacity);
    }

    void write(const std::string& str) {
        std::size_t len = std::min(str.size(), kCapacity - 1);
        std::memcpy(data_, str.data(), len);
        size_ = len;
    }

    void reset() {
        std::memset(data_, 0, size_);
        size_ = 0;
    }

    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] const char* data() const { return data_; }

private:
    char data_[kCapacity];
    std::size_t size_{0};
};

int main() {
    std::cout << "=== MiniCache Phase 3: ObjectPool Test Engine ===\n\n";

    constexpr std::size_t kInitialSize = 3;
    auto pool = std::make_shared<minicache::ObjectPool<NetworkBuffer>>(kInitialSize);

    assert(pool->available_count() == 3);
    std::cout << "[PASS] Initial pool capacity: " << pool->available_count() << '\n';

    // 1. Test Acquire and Auto-Recycle via Scope RAII
    {
        auto buf1 = pool->acquire();
        buf1->write("SET key1 value1");
        std::cout << "[PASS] Acquired buf1, pool available: " << pool->available_count() << '\n';
        assert(pool->available_count() == 2);

        auto buf2 = pool->acquire();
        buf2->write("GET key1");
        assert(pool->available_count() == 1);
    } // buf1 and buf2 recycled here

    assert(pool->available_count() == 3);
    std::cout << "[PASS] Buffers auto-recycled upon scope exit. Pool available: " 
              << pool->available_count() << '\n';

    // 2. Verify State Resetting (Scoped so recycled_buf is returned immediately)
    {
        auto recycled_buf = pool->acquire();
        assert(recycled_buf->size() == 0);
        std::cout << "[PASS] Recycled buffer successfully reset to clean state.\n";
    } // recycled_buf recycled here!

    assert(pool->available_count() == 3);

    // 3. Test Pool Exhaustion & Dynamic Expansion
    std::vector<minicache::ObjectPool<NetworkBuffer>::PooledPtr> held_buffers;
    for (int i = 0; i < 5; ++i) {
        held_buffers.push_back(pool->acquire());
    }

    std::cout << "[PASS] Acquired 5 buffers simultaneously (Pool expanded dynamically).\n";
    std::cout << "  Current available: " << pool->available_count() 
              << ", Total instances created: " << pool->total_created_count() << '\n';

    held_buffers.clear(); // Return all 5 buffers to pool
    assert(pool->available_count() == pool->total_created_count());
    std::cout << "[PASS] All buffers released. Pool count: " << pool->available_count() << '\n';

    std::cout << "\n=== All Phase 3 ObjectPool Tests Passed Successfully! ===\n";
    return 0;
}