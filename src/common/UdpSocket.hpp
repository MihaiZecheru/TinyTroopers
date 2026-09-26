#pragma once

#include "common/Constants.hpp"

#include <array>
#include <cstdint>
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
using SocketHandle = SOCKET;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
#endif

class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    bool Open(std::uint16_t port);
    void Close();
    bool Send(const sockaddr_in& address, const void* data, int size);
    int Receive(sockaddr_in& address, void* data, int size);
    static sockaddr_in MakeAddress(const std::string& host, std::uint16_t port);
    static std::string AddressKey(const sockaddr_in& address);

private:
    SocketHandle handle;
};
