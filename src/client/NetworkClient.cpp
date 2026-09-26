#include "client/NetworkClient.hpp"

#include "common/UdpSocket.hpp"

#include <array>
#include <cstring>

class NetworkClient::Impl {
public:
    UdpSocket socket;
    sockaddr_in serverAddress{};
    std::string lastMessage;
    bool connected = false;
};

NetworkClient::NetworkClient() : impl(std::make_unique<Impl>()) {}

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::Connect(const std::string& host, std::uint16_t port, const std::string& playerName, const std::string& roomCode, bool createRoom, bool preferCompact) {
    impl->connected = impl->socket.Open(0);
    if (!impl->connected) {
        return false;
    }
    impl->serverAddress = UdpSocket::MakeAddress(host, port);
    HelloPacket packet{};
    packet.header = MakeHeader(PacketType::Hello, sizeof(HelloPacket));
    std::strncpy(packet.name.data(), playerName.c_str(), packet.name.size() - 1);
    std::strncpy(packet.roomCode.data(), roomCode.c_str(), packet.roomCode.size() - 1);
    packet.createRoom = createRoom;
    packet.preferCompact = preferCompact;
    return impl->socket.Send(impl->serverAddress, &packet, sizeof(packet));
}

void NetworkClient::SendConfig(const LobbyConfigPacket& packet) {
    if (impl->connected) {
        impl->socket.Send(impl->serverAddress, &packet, sizeof(packet));
    }
}

void NetworkClient::SendInput(const InputPacket& packet) {
    if (impl->connected) {
        impl->socket.Send(impl->serverAddress, &packet, sizeof(packet));
    }
}

void NetworkClient::SendStart() {
    if (!impl->connected) {
        return;
    }
    StartMatchPacket packet{};
    packet.header = MakeHeader(PacketType::StartMatch, sizeof(StartMatchPacket));
    impl->socket.Send(impl->serverAddress, &packet, sizeof(packet));
}

void NetworkClient::SendKick(std::uint32_t targetPlayerId) {
    if (!impl->connected) {
        return;
    }
    KickPlayerPacket packet{};
    packet.header = MakeHeader(PacketType::KickPlayer, sizeof(KickPlayerPacket));
    packet.targetPlayerId = targetPlayerId;
    impl->socket.Send(impl->serverAddress, &packet, sizeof(packet));
}

void NetworkClient::Disconnect(std::uint32_t playerId) {
    if (!impl->connected) {
        return;
    }
    DisconnectPacket packet{};
    packet.header = MakeHeader(PacketType::Disconnect, sizeof(DisconnectPacket));
    packet.playerId = playerId;
    for (int i = 0; i < 3; ++i) {
        impl->socket.Send(impl->serverAddress, &packet, sizeof(packet));
    }
    impl->socket.Close();
    impl->connected = false;
    impl->lastMessage.clear();
}

bool NetworkClient::IsConnected() const {
    return impl->connected;
}

bool NetworkClient::PollSnapshot(SnapshotPacket& packet) {
    std::array<std::uint8_t, cfg::PacketBytes> bytes{};
    sockaddr_in sender{};
    bool found = false;
    while (true) {
        const int received = impl->socket.Receive(sender, bytes.data(), static_cast<int>(bytes.size()));
        if (received <= 0) {
            break;
        }
        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(bytes.data());
        if (received >= static_cast<int>(sizeof(SnapshotPacket)) && IsValidHeader(*header, PacketType::Snapshot)) {
            packet = *reinterpret_cast<const SnapshotPacket*>(bytes.data());
            found = true;
        } else if (received >= static_cast<int>(sizeof(PacketHeader)) && IsValidHeader(*header, PacketType::CompactSnapshot)) {
            if (DeserializeCompactSnapshot(bytes.data(), received, packet)) {
                found = true;
            }
        } else if (received >= static_cast<int>(sizeof(ServerMessagePacket)) && IsValidHeader(*header, PacketType::ServerMessage)) {
            const ServerMessagePacket* message = reinterpret_cast<const ServerMessagePacket*>(bytes.data());
            impl->lastMessage = message->text.data();
        }
    }
    return found;
}

bool NetworkClient::PollMessage(std::string& text) {
    if (impl->lastMessage.empty()) {
        return false;
    }
    text = impl->lastMessage;
    impl->lastMessage.clear();
    return true;
}
