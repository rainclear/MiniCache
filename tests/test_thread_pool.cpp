#include "common/thread_pool.hpp"
#include <iostream>
#include <cassert>
#include <atomic>
#include <chrono>

int main() {
    std::cout << "=== MiniCache ThreadPool Unit Test ===\n\n";

    // 1. Test Zero-Thread Fallback
    {
        minicache::ThreadPool zero_pool(0);
        assert(zero_pool.thread_count() == 2);
        std::cout << "[PASS] ThreadPool initialized with 0 threads fell back to 2 workers.\n";
    }

    constexpr std::size_t kNumThreads = 4;
    minicache::ThreadPool pool(kNumThreads);

    assert(pool.thread_count() == kNumThreads);
    std::cout << "[PASS] ThreadPool initialized with " << pool.thread_count() << " workers.\n";

    // 2. Test basic future return value task
    auto fut = pool.enqueue([]() {
        return 42;
    });
    assert(fut.get() == 42);
    std::cout << "[PASS] Basic task future returned expected result (42).\n";

    // 3. Test concurrent atomic increments and pending task inspection
    constexpr int kNumTasks = 100;
    std::atomic<int> counter{0};

    for (int i = 0; i < kNumTasks; ++i) {
        pool.enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Verify pending tasks query works
    std::cout << "[INFO] Enqueued " << kNumTasks << " tasks, initial pending tasks in queue: " 
              << pool.pending_tasks() << "\n";

    // Wait for tasks to complete
    while (counter.load() < kNumTasks) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    assert(counter.load() == kNumTasks);
    assert(pool.pending_tasks() == 0);
    std::cout << "[PASS] Executed " << kNumTasks << " concurrent tasks successfully.\n";

    // 4. Test explicit stop() call idempotency
    pool.stop();
    pool.stop(); // Calling stop() a second time should be safe
    std::cout << "[PASS] ThreadPool explicit stop() executed gracefully.\n";

    std::cout << "\n=== All ThreadPool Tests Passed Successfully! ===\n";
    return 0;
}