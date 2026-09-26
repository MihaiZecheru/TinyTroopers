#include "common/Protocol.hpp"

PacketHeader MakeHeader(PacketType type, std::uint16_t size) {
    return PacketHeader{cfg::ProtocolMagic, cfg::ProtocolVersion, type, size};
}

bool IsValidHeader(const PacketHeader& header, PacketType expectedType) {
    return header.magic == cfg::ProtocolMagic &&
        header.version == cfg::ProtocolVersion &&
        header.type == expectedType;
}
