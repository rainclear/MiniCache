#include "minicache/cache_store.hpp"
#include "minicache/aof_engine.hpp"
#include "minicache/command_factory.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

int main() {
    std::cout << "=== MiniCache AOF Persistence Test ===\n\n";

    const std::string aof_filename = "test_minicache.aof";

    // Clean up old test AOF file if it exists
    if (std::filesystem::exists(aof_filename)) {
        std::filesystem::remove(aof_filename);
    }

    // 1. Session 1: Write data and log mutating commands to AOF
    {
        minicache::CacheStore store(100, 0); // Disable background thread for test determinism
        minicache::AofEngine aof(aof_filename);

        // Execute commands via parser & log to AOF
        std::vector<std::string> commands = {
            "SET user:1001 \"Alice\"",
            "SET user:1002 \"Bob\"",
            "SET temp:key \"ToDelete\"",
            "DEL temp:key"
        };

        for (const auto& cmd_str : commands) {
            auto cmd = minicache::CommandFactory::parse(cmd_str);
            assert(cmd != nullptr);
            cmd->execute(store);
            aof.append(cmd_str);
        }

        assert(store.size() == 2);
        std::cout << "[PASS] Session 1 active cache size: " << store.size() << '\n';
    } // AofEngine and CacheStore go out of scope and flush to disk

    // 2. Session 2: Instantiate new empty CacheStore and replay commands from AOF file
    {
        minicache::CacheStore restored_store(100, 0);
        minicache::AofEngine aof(aof_filename);

        std::size_t replayed = aof.load(restored_store);
        std::cout << "[PASS] Replayed " << replayed << " commands from " << aof_filename << '\n';
        assert(replayed == 4);

        // Verify state recovery
        auto user1 = restored_store.get("user:1001");
        assert(user1.has_value() && *user1 == "Alice");

        auto user2 = restored_store.get("user:1002");
        assert(user2.has_value() && *user2 == "Bob");

        auto temp = restored_store.get("temp:key");
        assert(!temp.has_value()); // Was deleted prior to shutdown

        assert(restored_store.size() == 2);
        std::cout << "[PASS] Restored cache size: " << restored_store.size() << '\n';
    }

    // Cleanup generated test AOF file
    std::filesystem::remove(aof_filename);

    std::cout << "\n=== All AOF Persistence Tests Passed Successfully! ===\n";
    return 0;
}