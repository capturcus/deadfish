#pragma once
#include <unordered_map>
#include <cstdint>
#include <string>
#include <Box2D/Box2D.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <glm/vec2.hpp>
#include "deadfish_generated.h"

#define GLM_ENABLE_EXPERIMENTAL

// const float METERS2PIXELS = 100.f;
// const float PIXELS2METERS = 0.01f;

typedef websocketpp::server<websocketpp::config::asio> server;

std::ostream &operator<<(std::ostream &os, glm::vec2 &v);

static inline glm::vec2 b2g(b2Vec2 v) {
    return glm::vec2(v.x, v.y);
}

static inline b2Vec2 g2b(glm::vec2 v) {
    return b2Vec2(v.x, v.y);
}

enum class GamePhase {
    LOBBY = 0,
    GAME
};

struct Mob {
    uint16_t id = 0;
    uint16_t species = 0;
    b2Body* body = nullptr;

    glm::vec2 targetPosition;

    virtual void update();
};

struct Player : public Mob {
    std::string name;
    websocketpp::connection_hdl conn_hdl;

    void update() override;
};

struct Bush {
    glm::vec2 position;
    float radius = 0;
};

struct Stone {
    b2Body* body = nullptr;
};

struct Level {
    std::vector<std::unique_ptr<Bush>> bushes;
    std::vector<std::unique_ptr<Stone>> stones;
    std::vector<glm::vec2> navpoints;
    std::vector<glm::vec2> playerpoints;
    std::vector<glm::vec2> npcspawns;
    glm::vec2 size;
};

struct GameState {
    std::vector<std::unique_ptr<Player>> players;
    std::vector<std::unique_ptr<Mob>> npcs;
    GamePhase phase = GamePhase::LOBBY;
    int lastSpecies = 0;
    std::unique_ptr<Level> level = nullptr;

    std::unique_ptr<b2World> b2world = nullptr;
};

extern GameState gameState;
extern server websocket_server;