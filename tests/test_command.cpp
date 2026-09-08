#include "minicache/cache_store.hpp"
#include "minicache/command_factory.hpp"
#include <iostream>
#include <cassert>
#include <thread>

void execute_and_print(minicache::CacheStore& store, std::string_view raw_cmd) {
    auto cmd = minicache::CommandFactory::parse(raw_cmd);
    if (!cmd) {
        std::cout << "ERR: Invalid or Unknown Command -> \"" << raw_cmd << "\"\n";
        return;
    }
    std::string res = cmd->execute(store);
    std::cout << "> " << raw_cmd << "\n  " << res << "\n";
}

int main() {
    std::cout << "=== MiniCache Phase 2: Command Engine Test ===\n\n";

    minicache::CacheStore store(10);

    // 1. Test SET & GET with string quotes
    execute_and_print(store, "SET user:10086 \"Alice Zhang\"");
    execute_and_print(store, "GET user:10086");

    // 2. Test PING
    execute_and_print(store, "PING");

    // 3. Test TTL via SET Command (100ms)
    execute_and_print(store, "SET temp_key temp_val 100");
    execute_and_print(store, "PING");
    execute_and_print(store, "GET temp_key");

    std::cout << "\nSleeping 150ms...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    execute_and_print(store, "GET temp_key"); // Should yield (nil)

    // 4. Test DEL
    execute_and_print(store, "DEL user:10086");
    execute_and_print(store, "GET user:10086"); // Should yield (nil)

    // 5. Test Invalid Command
    execute_and_print(store, "INVALID_CMD foo bar");

    std::cout << "\n=== Phase 2 Command Engine Tests Passed! ===\n";
    return 0;
}