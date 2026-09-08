#include "minicache/command_factory.hpp"
#include <algorithm>
#include <cctype>

namespace minicache {

std::unordered_map<std::string, CommandFactory::Creator>& CommandFactory::get_registry() {
    static std::unordered_map<std::string, Creator> registry;
    
    // Auto-register built-in commands on first access
    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        // Register SET handler
        registry["SET"] = [](const std::vector<std::string>& args) -> std::unique_ptr<Command> {
            if (args.size() < 2) return nullptr;
            std::int64_t ttl = -1;
            if (args.size() >= 3) {
                try {
                    ttl = std::stoll(args[2]);
                } catch (...) {
                    return nullptr;
                }
            }
            return std::make_unique<SetCommand>(args[0], args[1], ttl);
        };

        // Register GET handler
        registry["GET"] = [](const std::vector<std::string>& args) -> std::unique_ptr<Command> {
            if (args.size() != 1) return nullptr;
            return std::make_unique<GetCommand>(args[0]);
        };

        // Register DEL handler
        registry["DEL"] = [](const std::vector<std::string>& args) -> std::unique_ptr<Command> {
            if (args.size() != 1) return nullptr;
            return std::make_unique<DelCommand>(args[0]);
        };

        // Register PING handler
        registry["PING"] = [](const std::vector<std::string>& args) -> std::unique_ptr<Command> {
            if (!args.empty()) return nullptr;
            return std::make_unique<PingCommand>();
        };
    }
    return registry;
}

void CommandFactory::register_command(const std::string& name, Creator creator) {
    std::string upper_name = name;
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    get_registry()[upper_name] = std::move(creator);
}

std::vector<std::string> CommandFactory::tokenize(std::string_view raw_cmd) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;

    for (char ch : raw_cmd) {
        if (ch == '"') {
            in_quotes = !in_quotes; // Toggle quote state for string literals
        } else if (std::isspace(static_cast<unsigned char>(ch)) && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

std::unique_ptr<Command> CommandFactory::parse(std::string_view raw_cmd) {
    auto tokens = tokenize(raw_cmd);
    if (tokens.empty()) {
        return nullptr;
    }

    std::string cmd_name = tokens[0];
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    auto& registry = get_registry();
    auto it = registry.find(cmd_name);
    if (it == registry.end()) {
        return nullptr; // Unknown command
    }

    // Pass rest arguments to the creator
    std::vector<std::string> args(
        std::make_move_iterator(tokens.begin() + 1),
        std::make_move_iterator(tokens.end())
    );

    return (it->second)(args);
}

// Append this inside namespace minicache in command_factory.cpp
std::unordered_map<std::string, ShardedCommandFactory::Creator>& ShardedCommandFactory::get_registry() {
    static std::unordered_map<std::string, Creator> registry;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;

        registry["SET"] = [](const std::vector<std::string>& args) -> std::unique_ptr<ShardedCommand> {
            if (args.size() < 2) return nullptr;
            std::int64_t ttl = -1;
            if (args.size() >= 3) {
                try { ttl = std::stoll(args[2]); } catch (...) { return nullptr; }
            }
            return std::make_unique<ShardedSetCommand>(args[0], args[1], ttl);
        };

        registry["GET"] = [](const std::vector<std::string>& args) -> std::unique_ptr<ShardedCommand> {
            if (args.size() != 1) return nullptr;
            return std::make_unique<ShardedGetCommand>(args[0]);
        };

        registry["DEL"] = [](const std::vector<std::string>& args) -> std::unique_ptr<ShardedCommand> {
            if (args.size() != 1) return nullptr;
            return std::make_unique<ShardedDelCommand>(args[0]);
        };

        registry["DBSIZE"] = [](const std::vector<std::string>& args) -> std::unique_ptr<ShardedCommand> {
            if (!args.empty()) return nullptr;
            return std::make_unique<ShardedSizeCommand>();
        };

        registry["EXISTS"] = [](const std::vector<std::string>& args) -> std::unique_ptr<ShardedCommand> {
            if (args.size() != 1) return nullptr;
            return std::make_unique<ShardedExistsCommand>(args[0]);
        };
    }
    return registry;
}

std::vector<std::string> ShardedCommandFactory::tokenize(std::string_view raw_cmd) {
    // Reuses same tokenization logic
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;

    for (char ch : raw_cmd) {
        if (ch == '"') {
            in_quotes = !in_quotes;
        } else if (std::isspace(static_cast<unsigned char>(ch)) && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

std::unique_ptr<ShardedCommand> ShardedCommandFactory::parse(std::string_view raw_cmd) {
    auto tokens = tokenize(raw_cmd);
    if (tokens.empty()) return nullptr;

    std::string cmd_name = tokens[0];
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    auto& registry = get_registry();
    auto it = registry.find(cmd_name);
    if (it == registry.end()) return nullptr;

    std::vector<std::string> args(
        std::make_move_iterator(tokens.begin() + 1),
        std::make_move_iterator(tokens.end())
    );

    return (it->second)(args);
}

} // namespace minicache