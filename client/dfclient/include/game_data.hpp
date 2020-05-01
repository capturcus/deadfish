#pragma once

#include <string>
#include <vector>

#include "websocket.hpp"

struct Player {
    std::string name;
    uint16_t species;
    bool ready;
};

struct GameData {
    std::string serverAddress;
    std::string myNickname;
    uint16_t myID;
    std::vector<Player> players;
    std::string levelData;

    WebSocket* socket = nullptr;
};

extern GameData gameData;
