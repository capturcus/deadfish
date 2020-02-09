#pragma once
#include <unordered_map>
#include <cstdint>
#include <string>
#include <mutex>
#include <memory>
#include <Box2D/Box2D.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <glm/vec2.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "deadfish_generated.h"

#define GLM_ENABLE_EXPERIMENTAL

// const float METERS2PIXELS = 100.f;
// const float PIXELS2METERS = 0.01f;
const std::string INI_PATH = "./deadfish.ini";

typedef websocketpp::server<websocketpp::config::asio> server;

std::ostream& operator<<(std::ostream &os, glm::vec2 &v);
std::ostream& operator<<(std::ostream &os, b2Vec2 v);
std::ostream& operator<<(std::ostream& os, std::vector<std::string>& v);

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

enum class MobState {
    WALKING = 0,
    RUNNING = 1,
    ATTACKING = 2
};

struct Collideable {
    bool toBeDeleted = false;
    virtual void handleCollision(Collideable* other) {}

    virtual ~Collideable(){}
};

struct Mob : public Collideable {
    uint16_t id = 0;
    uint16_t species = 0;
    b2Body* body = nullptr;
    MobState state = MobState::WALKING;
    virtual void handleCollision(Collideable* other) override {}

    glm::vec2 targetPosition;

    virtual bool update();
    virtual ~Mob();
};

struct Player : public Mob {
    std::string name;
    websocketpp::connection_hdl conn_hdl;
    bool ready = false;
    Mob* killTarget = nullptr;
    uint16_t attackTimeout = 0;
    std::chrono::system_clock::time_point lastAttack;
    int points = 0;
    uint16_t deathTimeout = 0;
    
    void handleCollision(Collideable* other) override;
    bool update() override;
    void reset();
};

struct Civilian : public Mob {
    std::string currentNavpoint;
    std::string previousNavpoint;

    bool update() override;
    void setNextNavpoint();
};

struct Bush {
    glm::vec2 position;
    float radius = 0;
};

struct Stone : public Collideable {
    b2Body* body = nullptr;
};

struct NavPoint {
    glm::vec2 position;
    std::vector<std::string> neighbors;
    bool isspawn;
    bool isplayerspawn;
};

struct Level {
    std::vector<std::unique_ptr<Bush>> bushes;
    std::vector<std::unique_ptr<Stone>> stones;
    std::unordered_map<std::string, std::unique_ptr<NavPoint>> navpoints;
    glm::vec2 size;
};

struct GameState {
private:
    std::mutex mut;

public:
    std::vector<std::unique_ptr<Player>> players;
    std::vector<std::unique_ptr<Civilian>> civilians;
    GamePhase phase = GamePhase::LOBBY;
    int lastSpecies = 0;
    std::unique_ptr<Level> level = nullptr;

    std::unique_ptr<b2World> b2world = nullptr;

    inline std::unique_ptr<std::lock_guard<std::mutex>> lock() {
        return std::make_unique<std::lock_guard<std::mutex>>(mut);
    }

    boost::property_tree::ptree config;
};

extern GameState gameState;
extern server websocket_server;