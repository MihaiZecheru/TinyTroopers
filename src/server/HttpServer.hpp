#pragma once

#include <cstdint>
#include <functional>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using TcpSocketHandle = SOCKET;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using TcpSocketHandle = int;
#endif

class HttpServer {
public:
    using RequestHandler = std::function<std::string(const std::string& path)>;

    HttpServer();
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    bool Open(std::uint16_t port);
    void Close();
    void Poll(const RequestHandler& handler);

private:
    TcpSocketHandle listenHandle;
};
