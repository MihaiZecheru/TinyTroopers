#include "server/GameServer.hpp"

int main() {
    GameServer server;
    return server.Run() ? 0 : 1;
}
