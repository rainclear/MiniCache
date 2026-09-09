#include "minicache/cache_store.hpp"
#include "minicache/aof_engine.hpp"
#include "minicache/server.hpp"
#include "minicache/object_pool.hpp"

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

    // 1. Initialize Storage Engine & AOF Persistence
    minicache::CacheStore store(10000, 500);
    minicache::AofEngine aof("minicache.aof");

    // 2. Replay AOF log if present
    std::cout << "[AOF] Replaying persistent logs from minicache.aof...\n";
    std::size_t replayed = aof.load(store);
    std::cout << "[AOF] Replayed " << replayed << " commands. Current cache size: " << store.size() << "\n";

    // 3. Initialize Shared ObjectPool for Network Buffers
    auto buffer_pool = std::make_shared<minicache::ObjectPool<minicache::NetworkBuffer>>(32);
    std::cout << "[Pool] Pre-allocated " << buffer_pool->available_count() << " network buffers in ObjectPool.\n";

    // 4. Start Server with Pooled Network I/O
    minicache::Server server(store, &aof, port, buffer_pool);
    server.start();

    std::cout << "[Server] MiniCache listening on 0.0.0.0:" << port << " (RESP compatible, Pooled I/O)\n";
    std::cout << "[Server] Press Ctrl+C to stop.\n\n";

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[Server] Stopping network loop...\n";
    server.stop();
    std::cout << "[AOF] Flushing WAL to disk...\n";
    aof.sync();

    std::cout << "[MiniCache] Shutdown complete. Bye!\n";
    return 0;
}