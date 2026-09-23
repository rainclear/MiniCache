#include "storage/cache_store.hpp"
#include "aof/aof_engine.hpp"
#include "server/server.hpp"
#include "common/object_pool.hpp"
#include "common/thread_pool.hpp"
#include "net/buffer.hpp"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[MiniCache] Signal " << signal << " received. Shutting down gracefully...\n";
        g_running = false;
    }
}

int main(int argc, char* argv[]) {
    int port = 6379;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "==================================================\n";
    std::cout << "           MiniCache In-Memory Engine            \n";
    std::cout << "==================================================\n";

    // 1. 初始化存储引擎与 AOF 持久化
    minicache::CacheStore store(10000, 500);
    minicache::AofEngine aof("minicache.aof");

    // 2. 重放 AOF 持久化日志
    std::cout << "[AOF] Replaying persistent logs from minicache.aof...\n";
    std::size_t replayed = aof.load(store);
    std::cout << "[AOF] Replayed " << replayed << " commands. Current cache size: " << store.size() << "\n";

    // 3. 初始化网络 Buffer 对象池
    auto buffer_pool = std::make_shared<minicache::ObjectPool<minicache::net::NetworkBuffer>>(32);
    std::cout << "[Pool] Pre-allocated " << buffer_pool->available_count() << " network buffers in ObjectPool.\n";

    // 4. 初始化工作线程池
    unsigned int num_workers = std::thread::hardware_concurrency();
    if (num_workers == 0) num_workers = 4;
    auto thread_pool = std::make_shared<minicache::ThreadPool>(num_workers);
    std::cout << "[ThreadPool] Spawned " << thread_pool->thread_count() << " worker threads.\n";

    // 5. 启动 Server（传入重构后的 net::NetworkBuffer 命名空间类型）
    minicache::Server server(store, &aof, port, buffer_pool, thread_pool);
    server.start();

    std::cout << "[Server] MiniCache listening on 0.0.0.0:" << port << " (RESP compatible, Pooled I/O, Multi-Threaded Execution)\n";
    std::cout << "[Server] Press Ctrl+C to stop.\n\n";

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[Server] Stopping network loop...\n";
    server.stop();
    std::cout << "[ThreadPool] Flushing pending worker tasks...\n";
    thread_pool->stop();
    std::cout << "[AOF] Flushing WAL to disk...\n";
    aof.sync();

    std::cout << "[MiniCache] Shutdown complete. Bye!\n";
    return 0;
}