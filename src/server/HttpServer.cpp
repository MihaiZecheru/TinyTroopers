#include "server/HttpServer.hpp"

#include <array>
#include <sstream>

namespace {
#ifdef _WIN32
constexpr TcpSocketHandle InvalidTcpSocket = INVALID_SOCKET;
#else
constexpr TcpSocketHandle InvalidTcpSocket = -1;
#endif
}

HttpServer::HttpServer() : listenHandle(InvalidTcpSocket) {}

HttpServer::~HttpServer() {
    Close();
}

bool HttpServer::Open(std::uint16_t port) {
    Close();
    listenHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenHandle == InvalidTcpSocket) {
        return false;
    }

    int opt = 1;
    setsockopt(listenHandle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

#ifdef _WIN32
    u_long nonBlocking = 1;
    ioctlsocket(listenHandle, FIONBIO, &nonBlocking);
#else
    const int flags = fcntl(listenHandle, F_GETFL, 0);
    fcntl(listenHandle, F_SETFL, flags | O_NONBLOCK);
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(listenHandle, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        Close();
        return false;
    }

    if (listen(listenHandle, SOMAXCONN) < 0) {
        Close();
        return false;
    }

    return true;
}

void HttpServer::Close() {
    if (listenHandle == InvalidTcpSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(listenHandle);
#else
    close(listenHandle);
#endif
    listenHandle = InvalidTcpSocket;
}

void HttpServer::Poll(const RequestHandler& handler) {
    if (listenHandle == InvalidTcpSocket) {
        return;
    }

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        TcpSocketHandle client = accept(listenHandle, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (client == InvalidTcpSocket) {
            break;
        }

        // Set accepted socket to blocking with a short timeout so recv doesn't fail with WSAEWOULDBLOCK
#ifdef _WIN32
        u_long blocking = 0;
        ioctlsocket(client, FIONBIO, &blocking);
        DWORD timeoutMs = 500;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
#else
        const int flags = fcntl(client, F_GETFL, 0);
        fcntl(client, F_SETFL, flags & ~O_NONBLOCK);
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif

        std::string request;
        std::array<char, 1024> buffer{};
        while (request.size() < 4096) {
            const int bytesRead = recv(client, buffer.data(), static_cast<int>(buffer.size()), 0);
            if (bytesRead <= 0) {
                break;
            }
            request.append(buffer.data(), bytesRead);
            if (request.find("\r\n\r\n") != std::string::npos || request.find("\n\n") != std::string::npos) {
                break;
            }
        }

        if (!request.empty()) {
            std::string path = "/";
            const auto firstSpace = request.find(' ');
            if (firstSpace != std::string::npos) {
                const auto secondSpace = request.find(' ', firstSpace + 1);
                if (secondSpace != std::string::npos) {
                    path = request.substr(firstSpace + 1, secondSpace - (firstSpace + 1));
                }
            }

            const std::string body = handler ? handler(path) : "{\"status\":\"ok\"}";
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json; charset=utf-8\r\n"
                     << "Access-Control-Allow-Origin: *\r\n"
                     << "Content-Length: " << body.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << body;

            const std::string responseStr = response.str();
            int totalSent = 0;
            const int toSend = static_cast<int>(responseStr.size());
            while (totalSent < toSend) {
                const int sent = send(client, responseStr.c_str() + totalSent, toSend - totalSent, 0);
                if (sent <= 0) {
                    break;
                }
                totalSent += sent;
            }
        }

#ifdef _WIN32
        shutdown(client, SD_SEND);
        char drain[256];
        while (recv(client, drain, sizeof(drain), 0) > 0) {}
        closesocket(client);
#else
        shutdown(client, SHUT_WR);
        char drain[256];
        while (recv(client, drain, sizeof(drain), 0) > 0) {}
        close(client);
#endif
    }
}
