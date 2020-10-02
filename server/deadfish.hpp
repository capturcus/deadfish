#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include <Box2D/Box2D.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <glm/vec2.hpp>
#include <boost/program_options.hpp>
#include "../common/deadfish_generated.h"
#include "../common/constants.hpp"

namespace boost_po = boost::program_options;

#define UNUSED __attribute__((unused)) 

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
	GAME
};

enum class MobState {
	WALKING = 0,
	RUNNING = 1,
	ATTACKING = 2
};

struct Collideable {
	bool toBeDeleted = false;
	virtual void handleCollision(UNUSED Collideable& other) {}
	virtual bool obstructsSight(Player*) = 0;

	virtual ~Collideable(){}

	Collideable(){}
	Collideable(const Collideable&) = delete;
};

struct Mob : public Collideable {
	uint16_t mobID = 0;
	uint16_t species = 0;
	b2Body* body = nullptr;
	MobState state = MobState::WALKING;
	virtual void handleCollision(UNUSED Collideable& other) override {}
	virtual void handleKill(Player& killer) = 0;
	virtual bool isDead() { return false; }
	virtual bool obstructsSight(Player*) override { return false; }

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
	uint16_t playerID = 0;

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

// just a data container to be able to send it later to clients
struct Tileset {
	std::string path;
	u_int16_t firstgid;
};

// same as above
struct Visible {
	glm::vec2 pos;
	float rotation;
	u_int16_t gid;
};

struct HidingSpot : public Collideable {
	b2Body* body = nullptr;
	float radius = 0;
	virtual bool obstructsSight(Player* p) override {
		auto distance = b2Distance(body->GetPosition(), p->body->GetPosition());
		if(distance < body->GetFixtureList()->GetShape()->m_radius) {
			return false;
		}
		return true;
	}
};

struct Collision : public Collideable {
	b2Body* body = nullptr;
	virtual bool obstructsSight(Player*) override { return true; }
};

struct PlayerWall : public Collideable {
	b2Body* body = nullptr;
	virtual bool obstructsSight(Player*) override { return false; }
};

struct NavPoint {
	glm::vec2 position;
	float radius;
	std::vector<std::string> neighbors;
	bool isspawn;
	bool isplayerspawn;
};

struct Level {
	std::vector<std::unique_ptr<Visible>> visible;
	std::vector<std::unique_ptr<Collision>> collisions;
	std::vector<std::unique_ptr<HidingSpot>> hidingspots;
	std::vector<std::unique_ptr<PlayerWall>> playerwalls;
	std::unordered_map<std::string, std::unique_ptr<NavPoint>> navpoints;
	glm::vec2 size;
};

struct GameState {
private:
	std::mutex mut;

public:
	GamePhase phase = GamePhase::LOBBY;
	std::unique_ptr<Level> level = nullptr;

	std::unique_ptr<b2World> b2world = nullptr;
	std::vector<std::unique_ptr<Player>> players;
	std::vector<std::unique_ptr<Civilian>> civilians;

	inline std::unique_ptr<std::lock_guard<std::mutex>> lock() {
		return std::make_unique<std::lock_guard<std::mutex>>(mut);
	}

	boost_po::variables_map options;
};

extern GameState gameState;
extern server websocket_server;