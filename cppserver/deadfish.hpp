#pragma once
#include <unordered_map>
#include <cstdint>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "deadfish_generated.h"
#include <glm/vec2.hpp>

#define GLM_ENABLE_EXPERIMENTAL

typedef websocketpp::server<websocketpp::config::asio> server;

std::ostream &operator<<(std::ostream &os, glm::vec2 &v);

enum class GamePhase {
    LOBBY = 0,
    GAME
};

struct Mob {
    uint16_t id = 0;
    glm::vec2 position = glm::vec2(100, 100);
    float angle;
    uint16_t species = 0;

    glm::vec2 targetPosition;

    virtual void update();
};

struct Player : public Mob {
    std::string name;
    websocketpp::connection_hdl conn_hdl;

    void update() override;
};

struct GameState {
    std::vector<std::unique_ptr<Player>> players;
    std::vector<std::unique_ptr<Mob>> npcs;
    GamePhase phase = GamePhase::LOBBY;
    int lastSpecies = 0;
};

extern GameState gameState;
extern server websocket_server;