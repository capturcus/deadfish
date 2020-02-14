#pragma once
#include <unordered_map>
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
#include "constants.hpp"

#define GLM_ENABLE_EXPERIMENTAL

// const float METERS2PIXELS = 100.f;
// const float PIXELS2METERS = 0.01f;
const std::string INI_PATH = "./deadfish.ini";

typedef websocketpp::server<websocketpp::config::asio> server;

std::ostream& operator<<(std::ostream &os, glm::vec2 &v);
std::ostream& operator<<(std::ostream &os, b2Vec2 v);
std::ostream& operator<<(std::ostream& os, std::vector<std::string>& v);

struct Player;

static inline glm::vec2 b2g(b2Vec2 v) {
    return glm::vec2(v.x, v.y);
}

static inline b2Vec2 g2b(glm::vec2 v) {
    return b2Vec2(v.x, v.y);
}

enum class GamePhase {
    LOBBY = 0,
    GAME,
    EXITING
};

enum class MobState {
    WALKING = 0,
    RUNNING = 1,
    ATTACKING = 2
};

struct Collideable {
    bool toBeDeleted = false;
    virtual void handleCollision(Collideable& other) {}
    virtual bool obstructsSight() = 0;

    virtual ~Collideable(){}

    Collideable(){}
    Collideable(const Collideable&) = delete;
};

struct Mob : public Collideable {
    uint16_t id = 0;
    uint16_t species = 0;
    b2Body* body = nullptr;
    MobState state = MobState::WALKING;
    virtual void handleCollision(Collideable& other) override {}
    virtual void handleKill(Player& killer) = 0;
    virtual bool isDead() { return false; }
    virtual bool obstructsSight() override { return false; }

    glm::vec2 targetPosition;

    virtual void update();
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
    
    void handleCollision(Collideable& other) override;
    void handleKill(Player& killer) override;
    void update() override;
    void reset();
    bool isDead() override;
    void setAttacking();
};

struct Civilian : public Mob {
    std::string currentNavpoint;
    std::string previousNavpoint;
    int slowFrames = 0;
    b2Vec2 lastPos;

    void handleKill(Player& killer) override;
    void update() override;
    void setNextNavpoint();
    void collisionResolution();
};

struct Bush {
    glm::vec2 position;
    float radius = 0;
};

struct Stone : public Collideable {
    b2Body* body = nullptr;
    virtual bool obstructsSight() override { return true; }
};

struct PlayerWall : public Collideable {
    b2Body* body = nullptr;
    virtual bool obstructsSight() override { return false; }
};

struct NavPoint {
    glm::vec2 position;
    float radius;
    std::vector<std::string> neighbors;
    bool isspawn;
    bool isplayerspawn;
};

struct Level {
    std::vector<std::unique_ptr<Bush>> bushes;
    std::vector<std::unique_ptr<Stone>> stones;
    std::vector<std::unique_ptr<PlayerWall>> playerwalls;
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
    std::unique_ptr<Level> level = nullptr;

    std::unique_ptr<b2World> b2world = nullptr;

    inline std::unique_ptr<std::lock_guard<std::mutex>> lock() {
        return std::make_unique<std::lock_guard<std::mutex>>(mut);
    }

    boost::property_tree::ptree config;
};

extern GameState gameState;
extern server websocket_server;