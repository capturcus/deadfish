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

struct GameData {
    std::string serverAddress;
    std::string myNickname;
    uint16_t myMobID;
    uint16_t myPlayerID;
    std::vector<Player> players;
    std::string levelData;
    bool gameInProgress = false;

    WebSocket* socket = nullptr;
};

extern GameData gameData;
