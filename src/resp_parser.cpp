#include "minicache/resp_parser.hpp"
#include <sstream>
#include <vector>

namespace minicache {

std::string RespParser::parse_array_to_cmd(std::string_view raw_resp) {
    if (raw_resp.empty() || raw_resp[0] != '*') {
        // Fallback: If not formatted as RESP array, treat as raw plain-text command line
        std::string plain_cmd(raw_resp);
        if (!plain_cmd.empty() && plain_cmd.back() == '\n') plain_cmd.pop_back();
        if (!plain_cmd.empty() && plain_cmd.back() == '\r') plain_cmd.pop_back();
        return plain_cmd;
    }

    std::size_t pos = 1;
    std::size_t crlf = raw_resp.find("\r\n", pos);
    if (crlf == std::string_view::npos) return "";

    int count = std::stoi(std::string(raw_resp.substr(pos, crlf - pos)));
    pos = crlf + 2;

    std::ostringstream cmd_builder;
    for (int i = 0; i < count; ++i) {
        if (pos >= raw_resp.size() || raw_resp[pos] != '$') return "";
        pos++; // Skip '$'

        std::size_t len_crlf = raw_resp.find("\r\n", pos);
        if (len_crlf == std::string_view::npos) return "";

        int str_len = std::stoi(std::string(raw_resp.substr(pos, len_crlf - pos)));
        pos = len_crlf + 2;

        if (pos + str_len > raw_resp.size()) return "";

        std::string_view token = raw_resp.substr(pos, str_len);
        pos += str_len + 2; // Skip trailing \r\n

        if (i > 0) cmd_builder << " ";
        cmd_builder << token;
    }

    return cmd_builder.str();
}

std::string RespParser::serialize_simple_string(std::string_view str) {
    return "+" + std::string(str) + "\r\n";
}

std::string RespParser::serialize_bulk_string(std::string_view str) {
    return "$" + std::to_string(str.size()) + "\r\n" + std::string(str) + "\r\n";
}

std::string RespParser::serialize_null() {
    return "$-1\r\n";
}

std::string RespParser::serialize_error(std::string_view err) {
    return "-ERR " + std::string(err) + "\r\n";
}

} // namespace minicache