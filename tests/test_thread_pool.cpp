#include "minicache/thread_pool.hpp"
#include <iostream>
#include <cassert>
#include <atomic>
#include <chrono>

int main() {
    std::cout << "=== MiniCache ThreadPool Unit Test ===\n\n";

    constexpr std::size_t kNumThreads = 4;
    minicache::ThreadPool pool(kNumThreads);

    assert(pool.thread_count() == kNumThreads);
    std::cout << "[PASS] ThreadPool initialized with " << pool.thread_count() << " workers.\n";

    // 1. Test basic future return value task
    auto fut = pool.enqueue([]() {
        return 42;
    });
    assert(fut.get() == 42);
    std::cout << "[PASS] Basic task future returned expected result (42).\n";

    // 2. Test concurrent atomic increments
    constexpr int kNumTasks = 100;
    std::atomic<int> counter{0};

    for (int i = 0; i < kNumTasks; ++i) {
        pool.enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Wait for tasks to complete
    while (counter.load() < kNumTasks) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    assert(counter.load() == kNumTasks);
    std::cout << "[PASS] Executed " << kNumTasks << " concurrent tasks successfully.\n";

    std::cout << "\n=== ThreadPool Tests Passed Successfully! ===\n";
    return 0;
}