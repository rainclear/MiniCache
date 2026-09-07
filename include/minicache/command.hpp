#ifndef MINICACHE_COMMAND_HPP
#define MINICACHE_COMMAND_HPP

#include "minicache/cache_store.hpp"
#include <string>
#include <memory>

namespace minicache {

/**
 * @brief Abstract Base Class representing an executable command.
 */
class Command {
public:
    virtual ~Command() = default;

    /**
     * @brief Executes the command against the target CacheStore instance.
     * @return Formatted execution result string.
     */
    virtual std::string execute(CacheStore& store) = 0;
};

/**
 * @brief Command to insert/update a key-value pair: SET <key> <val> [ttl_ms]
 */
class SetCommand : public Command {
public:
    SetCommand(std::string key, std::string value, std::int64_t ttl_ms = -1)
        : key_(std::move(key)), value_(std::move(value)), ttl_ms_(ttl_ms) {}

    std::string execute(CacheStore& store) override {
        store.set(key_, value_, ttl_ms_);
        return "OK";
    }

private:
    std::string key_;
    std::string value_;
    std::int64_t ttl_ms_;
};

/**
 * @brief Command to retrieve a value: GET <key>
 */
class GetCommand : public Command {
public:
    explicit GetCommand(std::string key) : key_(std::move(key)) {}

    std::string execute(CacheStore& store) override {
        auto result = store.get(key_);
        if (result.has_value()) {
            return *result;
        }
        return "(nil)";
    }

private:
    std::string key_;
};

/**
 * @brief Command to remove a key: DEL <key>
 */
class DelCommand : public Command {
public:
    explicit DelCommand(std::string key) : key_(std::move(key)) {}

    std::string execute(CacheStore& store) override {
        bool deleted = store.del(key_);
        return deleted ? "(integer) 1" : "(integer) 0";
    }

private:
    std::string key_;
};

} // namespace minicache

#endif // MINICACHE_COMMAND_HPP