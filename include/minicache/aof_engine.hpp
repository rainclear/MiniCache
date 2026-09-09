#ifndef MINICACHE_AOF_ENGINE_HPP
#define MINICACHE_AOF_ENGINE_HPP

#include "minicache/cache_store.hpp"
#include <string>
#include <fstream>
#include <mutex>

namespace minicache {

/**
 * @brief Manages Append-Only File (AOF) persistence and replay operations.
 */
class AofEngine {
public:
    explicit AofEngine(std::string filename = "minicache.aof");
    ~AofEngine();

    // Non-copyable, non-movable
    AofEngine(const AofEngine&) = delete;
    AofEngine& operator=(const AofEngine&) = delete;
    AofEngine(AofEngine&&) = delete;
    AofEngine& operator=(AofEngine&&) = delete;

    /**
     * @brief Appends a mutating command string to the AOF log file.
     */
    void append(const std::string& raw_cmd);

    /**
     * @brief Reads and replays all commands from the AOF file into a CacheStore.
     * @return Number of commands successfully replayed.
     */
    std::size_t load(CacheStore& store);

    /**
     * @brief Flushes pending writes and closes the file stream.
     */
    void sync();

private:
    std::string filename_;
    std::ofstream file_;
    std::mutex mutex_;
};

} // namespace minicache

#endif // MINICACHE_AOF_ENGINE_HPP
