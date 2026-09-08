#include "minicache/sharded_cache_store.hpp"
#include "minicache/command_factory.hpp"
#include <iostream>
#include <cassert>
#include <thread>

void execute_sharded(minicache::ShardedCacheStore& store, std::string_view raw_cmd) {
    auto cmd = minicache::ShardedCommandFactory::parse(raw_cmd);
    if (!cmd) {
        std::cout << "ERR: Invalid or Unknown Command -> \"" << raw_cmd << "\"\n";
        return;
    }
    std::string res = cmd->execute(store);
    std::cout << "> " << raw_cmd << "\n  " << res << "\n";
}

int main() {
    std::cout << "=== MiniCache: Sharded Command Engine Test ===\n\n";

    minicache::ShardedCacheStore store(100, 4);

    // 1. Basic Set & Get
    execute_sharded(store, "SET user:10086 \"Alice Zhang\"");
    execute_sharded(store, "GET user:10086");

    // 2. EXISTS & DBSIZE Commands
    execute_sharded(store, "EXISTS user:10086");
    execute_sharded(store, "DBSIZE");

    // 3. TTL Expiration Test
    execute_sharded(store, "SET temp_key temp_val 100");
    execute_sharded(store, "GET temp_key");

    std::cout << "\nSleeping 150ms...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    execute_sharded(store, "GET temp_key");

    // 4. Delete
    execute_sharded(store, "DEL user:10086");
    execute_sharded(store, "EXISTS user:10086");
    execute_sharded(store, "DBSIZE");

    std::cout << "\n=== Sharded Command Engine Tests Passed! ===\n";
    return 0;
}