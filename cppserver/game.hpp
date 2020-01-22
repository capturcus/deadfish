#pragma once
#include <cstdint>
#include <string>
#include <websocketpp/server.hpp>
#include "deadfish_generated.h"

enum class GamePhase {
    LOBBY = 0,
    GAME
};

struct Player {
    uint16_t id = 0;
    std::string name;
    DeadFish::Vec2 position;
    uint16_t species = 0;
    websocketpp::connection_hdl conn_hdl;
};

struct GameState {
    std::unordered_map<uint16_t, std::unique_ptr<Player>> players;
    GamePhase phase = GamePhase::LOBBY;
    int lastPlayerID = 0;
};

extern GameState gameState;