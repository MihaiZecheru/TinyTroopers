#include "common/UdpSocket.hpp"

#include <sstream>

#ifdef _WIN32
namespace {
class WinSockRuntime {
public:
    WinSockRuntime() {
        WSADATA data{};
        WSAStartup(MAKEWORD(2, 2), &data);
    }

    ~WinSockRuntime() {
        WSACleanup();
    }
};

WinSockRuntime winsockRuntime;
constexpr SocketHandle InvalidSocket = INVALID_SOCKET;
}
#else
namespace {
constexpr SocketHandle InvalidSocket = -1;
}
#endif

UdpSocket::UdpSocket() : handle(InvalidSocket) {}

UdpSocket::~UdpSocket() {
    Close();
}

bool UdpSocket::Open(std::uint16_t port) {
    Close();
    handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (handle == InvalidSocket) {
        return false;
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(port);

    if (bind(handle, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0) {
        Close();
        return false;
    }

#ifdef _WIN32
    u_long nonBlocking = 1;
    ioctlsocket(handle, FIONBIO, &nonBlocking);
#else
    const int flags = fcntl(handle, F_GETFL, 0);
    fcntl(handle, F_SETFL, flags | O_NONBLOCK);
#endif
    return true;
}

void UdpSocket::Close() {
    if (handle == InvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(handle);
#else
    close(handle);
#endif
    handle = InvalidSocket;
}

bool UdpSocket::Send(const sockaddr_in& address, const void* data, int size) {
    if (handle == InvalidSocket) {
        return false;
    }
    const int sent = sendto(handle, reinterpret_cast<const char*>(data), size, 0,
        reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    return sent == size;
}

int UdpSocket::Receive(sockaddr_in& address, void* data, int size) {
    if (handle == InvalidSocket) {
        return 0;
    }
    socklen_t addressSize = sizeof(address);
    const int received = recvfrom(handle, reinterpret_cast<char*>(data), size, 0,
        reinterpret_cast<sockaddr*>(&address), &addressSize);
    return received > 0 ? received : 0;
}

sockaddr_in UdpSocket::MakeAddress(const std::string& rawHost, std::uint16_t defaultPort) {
    std::string host = rawHost;
    std::uint16_t port = defaultPort;

    // Strip http:// or https:// prefix
    if (host.rfind("https://", 0) == 0) {
        host = host.substr(8);
    } else if (host.rfind("http://", 0) == 0) {
        host = host.substr(7);
    }

    // Strip trailing slash or URL path
    const auto slashPos = host.find('/');
    if (slashPos != std::string::npos) {
        host = host.substr(0, slashPos);
    }

    // Strip and parse custom port if specified (e.g., host:42069)
    const auto colonPos = host.rfind(':');
    if (colonPos != std::string::npos) {
        const std::string portPart = host.substr(colonPos + 1);
        try {
            port = static_cast<std::uint16_t>(std::stoul(portPart));
            host = host.substr(0, colonPos);
        } catch (...) {}
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) <= 0) {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        addrinfo* result = nullptr;
        const std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) == 0 && result != nullptr) {
            address = *reinterpret_cast<sockaddr_in*>(result->ai_addr);
            freeaddrinfo(result);
        }
    }
    return address;
}

std::string UdpSocket::AddressKey(const sockaddr_in& address) {
    std::array<char, cfg::AddressBytes> host{};
    inet_ntop(AF_INET, &address.sin_addr, host.data(), static_cast<socklen_t>(host.size()));
    std::ostringstream stream;
    stream << host.data() << ':' << ntohs(address.sin_port);
    return stream.str();
}
