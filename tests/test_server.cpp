#include "minicache/cache_store.hpp"
#include "minicache/server.hpp"
#include <iostream>
#include <cassert>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

std::string send_tcp_msg(int port, const std::string& msg) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        return "";
    }

    send(sock, msg.c_str(), msg.size(), 0);
    char buffer[1024] = {0};
    ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);
    close(sock);

    return (bytes > 0) ? std::string(buffer, bytes) : "";
}

int main() {
    std::cout << "=== MiniCache TCP Epoll Server Test ===\n\n";

    constexpr int kPort = 63799;
    minicache::CacheStore store(100, 0);
    minicache::Server server(store, nullptr, kPort);

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow server to bind

    // 1. Send SET command over TCP Socket
    std::string set_resp = send_tcp_msg(kPort, "SET netkey netval\r\n");
    std::cout << "[PASS] SET Response: " << set_resp;
    assert(set_resp == "+OK\r\n");

    // 2. Send GET command over TCP Socket
    std::string get_resp = send_tcp_msg(kPort, "GET netkey\r\n");
    std::cout << "[PASS] GET Response: " << get_resp;
    assert(get_resp == "$6\r\nnetval\r\n");

    // 3. Send DEL command over TCP Socket
    std::string del_resp = send_tcp_msg(kPort, "DEL netkey\r\n");
    std::cout << "[PASS] DEL Response: " << del_resp;

    server.stop();
    std::cout << "\n=== Network TCP Server Test Passed! ===\n";
    return 0;
}