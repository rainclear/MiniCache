#ifndef MINICACHE_COMMAND_FACTORY_HPP
#define MINICACHE_COMMAND_FACTORY_HPP

#include "minicache/command.hpp"
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <string_view>

namespace minicache {

/**
 * @brief Factory class to parse text instructions and instantiate Command objects.
 */
class CommandFactory {
public:
    using Creator = std::function<std::unique_ptr<Command>(const std::vector<std::string>& args)>;

    CommandFactory() = delete;

    /**
     * @brief Parses raw instruction text and constructs the corresponding Command instance.
     * @param raw_cmd e.g., "SET user:10086 \"Alice Zhang\" 5000"
     * @return std::unique_ptr<Command> or nullptr if parsing fails.
     */
    [[nodiscard]] static std::unique_ptr<Command> parse(std::string_view raw_cmd);

    /**
     * @brief Registers a new command handler into the factory registry.
     */
    static void register_command(const std::string& name, Creator creator);

private:
    static std::vector<std::string> tokenize(std::string_view raw_cmd);
    static std::unordered_map<std::string, Creator>& get_registry();
};

} // namespace minicache

#endif // MINICACHE_COMMAND_FACTORY_HPP