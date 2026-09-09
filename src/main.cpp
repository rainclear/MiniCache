#include "minicache/cache_store.hpp"
#include "minicache/aof_engine.hpp"
#include "minicache/server.hpp"

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

    // Register OS signal handlers for graceful exit
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "==================================================\n";
    std::cout << "           MiniCache In-Memory Engine            \n";
    std::cout << "==================================================\n";

    // 1. Initialize Storage Engine & AOF Persistence
    minicache::CacheStore store(10000, 500); // 10k capacity, 500ms active TTL purge
    minicache::AofEngine aof("minicache.aof");

    // 2. Replay AOF log if it exists
    std::cout << "[AOF] Replaying persistent logs from minicache.aof...\n";
    std::size_t replayed = aof.load(store);
    std::cout << "[AOF] Replayed " << replayed << " commands. Current cache size: " << store.size() << "\n";

    // 3. Start Non-Blocking Epoll TCP Server
    minicache::Server server(store, &aof, port);
    server.start();

    std::cout << "[Server] MiniCache listening on 0.0.0.0:" << port << " (Redis RESP compatible)\n";
    std::cout << "[Server] Press Ctrl+C to stop.\n\n";

    // Keep main thread alive until signal received
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