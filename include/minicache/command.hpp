#ifndef MINICACHE_COMMAND_HPP
#define MINICACHE_COMMAND_HPP

#include "minicache/cache_store.hpp"
#include "minicache/sharded_cache_store.hpp"
#include <string>
#include <memory>

namespace minicache {

// ==========================================
// Generic / Legacy Command Base Interface
// ==========================================

template <typename StoreType>
class CommandTemplate {
public:
    virtual ~CommandTemplate() = default;
    virtual std::string execute(StoreType& store) = 0;
};

// Backward-compatible alias for single CacheStore
using Command = CommandTemplate<CacheStore>;

// Legacy Commands for CacheStore
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

class GetCommand : public Command {
public:
    explicit GetCommand(std::string key) : key_(std::move(key)) {}

    std::string execute(CacheStore& store) override {
        auto result = store.get(key_);
        return result.has_value() ? *result : "(nil)";
    }

private:
    std::string key_;
};

class DelCommand : public Command {
public:
    explicit DelCommand(std::string key) : key_(std::move(key)) {}

    std::string execute(CacheStore& store) override {
        return store.del(key_) ? "(integer) 1" : "(integer) 0";
    }

private:
    std::string key_;
};

/**
 * @brief Command to test connection or echo text: PING [message]
 */
class PingCommand : public Command {
public:
    explicit PingCommand(std::string message = "PONG") 
        : message_(std::move(message)) {}

    std::string execute(CacheStore& store) override {
        // PING doesn't mutate store; it just returns the message/PONG and size of the store
        auto size = store.size();
        return message_ + " (" + std::to_string(size) + ")";
    }

private:
    std::string message_;
};

// ==========================================
// New Sharded Commands Interface
// ==========================================

using ShardedCommand = CommandTemplate<ShardedCacheStore>;

class ShardedSetCommand : public ShardedCommand {
public:
    ShardedSetCommand(std::string key, std::string value, std::int64_t ttl_ms = -1)
        : key_(std::move(key)), value_(std::move(value)), ttl_ms_(ttl_ms) {}

    std::string execute(ShardedCacheStore& store) override {
        store.set(key_, value_, ttl_ms_);
        return "OK";
    }

private:
    std::string key_;
    std::string value_;
    std::int64_t ttl_ms_;
};

class ShardedGetCommand : public ShardedCommand {
public:
    explicit ShardedGetCommand(std::string key) : key_(std::move(key)) {}

    std::string execute(ShardedCacheStore& store) override {
        auto result = store.get(key_);
        return result.has_value() ? *result : "(nil)";
    }

private:
    std::string key_;
};

class ShardedDelCommand : public ShardedCommand {
public:
    explicit ShardedDelCommand(std::string key) : key_(std::move(key)) {}

    std::string execute(ShardedCacheStore& store) override {
        return store.del(key_) ? "(integer) 1" : "(integer) 0";
    }

private:
    std::string key_;
};

// Additional Command: DBSIZE
class ShardedSizeCommand : public ShardedCommand {
public:
    std::string execute(ShardedCacheStore& store) override {
        return "(integer) " + std::to_string(store.size());
    }
};

// Additional Command: EXISTS <key>
class ShardedExistsCommand : public ShardedCommand {
public:
    explicit ShardedExistsCommand(std::string key) : key_(std::move(key)) {}

    std::string execute(ShardedCacheStore& store) override {
        auto res = store.get(key_);
        return res.has_value() ? "(integer) 1" : "(integer) 0";
    }

private:
    std::string key_;
};

} // namespace minicache

#endif // MINICACHE_COMMAND_HPP