#pragma once

#include <string>
#include <vector>

#include "websocket.hpp"

struct Player {
    std::string name;
    uint16_t species;
    uint16_t playerID;
    int16_t score;
    bool ready;
};

struct ChatEntry {
    uint16_t playerID;
    std::string message;
};

struct GameData {
    std::string serverAddress;
    std::string myNickname;
    uint16_t myMobID;
    uint16_t myPlayerID;
    std::vector<Player> players;
    std::string levelData;
    std::vector<ChatEntry> chatData;

    WebSocket* socket = nullptr;
};

extern GameData gameData;
