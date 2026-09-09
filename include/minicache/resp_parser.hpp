#ifndef MINICACHE_RESP_PARSER_HPP
#define MINICACHE_RESP_PARSER_HPP

#include <string>
#include <vector>
#include <string_view>

namespace minicache {

/**
 * @brief Utility class for Redis Serialization Protocol (RESP) formatting and parsing.
 */
class RespParser {
public:
    /**
     * @brief Parses RESP Array format (*3\r\n$3\r\nSET...) into a space-separated command string.
     * @return Command string if parsed successfully; std::nullopt otherwise.
     */
    static std::string parse_array_to_cmd(std::string_view raw_resp);

    /**
     * @brief Encapsulates a plain string response into a RESP Simple String (+OK\r\n).
     */
    static std::string serialize_simple_string(std::string_view str);

    /**
     * @brief Encapsulates a bulk string into RESP Bulk String format ($5\r\nhello\r\n).
     */
    static std::string serialize_bulk_string(std::string_view str);

    /**
     * @brief Formats null response into RESP Null Bulk String ($-1\r\n).
     */
    static std::string serialize_null();

    /**
     * @brief Formats error string into RESP Error format (-ERR ...\r\n).
     */
    static std::string serialize_error(std::string_view err);
};

} // namespace minicache

#endif // MINICACHE_RESP_PARSER_HPP