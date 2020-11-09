#pragma once
#include <unordered_map>
#include <map>
#include <string>
#include <mutex>
#include <memory>
#include <iostream>
#include <thread>
#include <Box2D/Box2D.h>
#include <glm/vec2.hpp>
#include <boost/program_options.hpp>
#include "../common/deadfish_generated.h"
#include "../common/constants.hpp"
#include "websocket.hpp"

namespace boost_po = boost::program_options;

#define UNUSED __attribute__((unused)) 

std::ostream& operator<<(std::ostream &os, glm::vec2 &v);
std::ostream& operator<<(std::ostream &os, b2Vec2 v);
std::ostream& operator<<(std::ostream& os, std::vector<std::string>& v);

const float INK_BOMB_SPEED_MODIFIER = 0.2f;

struct Player;

static inline glm::vec2 b2g(b2Vec2 v) {
	return glm::vec2(v.x, v.y);
}

static inline b2Vec2 g2b(glm::vec2 v) {
	return b2Vec2(v.x, v.y);
}

static inline FlatBuffGenerated::Vec2 b2f(b2Vec2 v) {
	return FlatBuffGenerated::Vec2(v.x, v.y);
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
	virtual void endCollision(UNUSED Collideable& other) {}

	virtual bool obstructsSight(Player*) = 0;

	b2Body* body = nullptr;

	virtual ~Collideable() {}

	virtual void update() {}

	Collideable(){}
	Collideable(const Collideable&) = delete;
};

struct Mob : public Collideable {
	uint16_t mobID = 0;
	uint16_t species = 0;
	MobState state = MobState::WALKING;
	virtual void handleCollision(UNUSED Collideable& other) override {}
	virtual void handleKill(Player& killer) = 0;
	virtual bool isDead() { return false; }
	virtual bool obstructsSight(Player*) override { return false; }
	virtual void update() override;
	virtual float calculateSpeed();

	glm::vec2 targetPosition;
	uint32_t bombsAffecting = 0;

	virtual ~Mob();
};

struct InkParticle :
	public Collideable {
	
	uint16_t inkID;
	uint16_t lifetimeFrames;

	InkParticle(b2Body* b);

	void handleCollision(Collideable& other) override;
	void endCollision(Collideable& other) override;
	bool obstructsSight(Player*) override;

	virtual void update() override;

	~InkParticle();
};

struct Player : public Mob {
	std::string name;
	bool ready = false;
	Mob* killTarget = nullptr;
	dfws::Handle wsHandle;
	uint16_t attackTimeout = 0;
	std::chrono::system_clock::time_point lastAttack;
	int points = 0;
	uint16_t deathTimeout = 0;
	uint16_t playerID = 0;
	std::vector<uint16_t> skills;

	float calculateSpeed() override;
	void handleCollision(Collideable& other) override;
	void handleKill(Player& killer) override;
	void update() override;
	void reset();
	bool isDead() override;
	void setAttacking();
	void sendSkillBarUpdate();
};

struct Civilian : public Mob {
	std::string currentNavpoint;
	std::string previousNavpoint;
	int slowFrames = 0;
	b2Vec2 lastPos;
	bool seenAManip;

	void handleKill(Player& killer) override;
	void update() override;
	void setNextNavpoint();
	void collisionResolution();
};

// just a data container to be able to send it later to clients
struct Tileinfo {
	Tileinfo(const FlatBuffGenerated::Tileinfo* fb_Ti) : name(fb_Ti->name()->str()), gid(fb_Ti->gid()) {}
	std::string name;
	uint16_t gid;
};

// just a data container to be able to send it later to clients
struct Object {
	Object(const FlatBuffGenerated::Object* fb_Obj) : pos(fb_Obj->pos()->x(), fb_Obj->pos()->y()),
		rotation(fb_Obj->rotation()), size(fb_Obj->size()->x(), fb_Obj->size()->y()),
		gid(fb_Obj->gid()), hspotname(fb_Obj->hspotname()->str()) {}
	glm::vec2 pos;
	float rotation;
	glm::vec2 size;
	uint16_t gid;
	std::string hspotname;
};

// just a data container to be able to send it later to clients
struct Decoration {
	Decoration(const FlatBuffGenerated::Decoration* fb_Dec) : pos(fb_Dec->pos()->x(), fb_Dec->pos()->y()),
		rotation(fb_Dec->rotation()), size(fb_Dec->size()->x(), fb_Dec->size()->y()), gid(fb_Dec->gid()) {}
	glm::vec2 pos;
	float rotation;
	glm::vec2 size;
	uint16_t gid;
};

// just a data container to be able to send it later to clients
struct Tilelayer {
	Tilelayer(const FlatBuffGenerated::Tilelayer* fb_Tl) : width(fb_Tl->width()), height(fb_Tl->height()),
		tilesize(fb_Tl->tilesize()->x(), fb_Tl->tilesize()->y()), tiledata(fb_Tl->tiledata()->begin(), fb_Tl->tiledata()->end()) {}
	uint16_t width;
	uint16_t height;
	glm::vec2 tilesize;
	std::vector<uint16_t> tiledata;
};

struct HidingSpot : public Collideable {
	HidingSpot(const FlatBuffGenerated::HidingSpot*);
	std::string name;
	std::set<Player*> playersInside;
	virtual void handleCollision(Collideable& other) override;
	virtual void endCollision(Collideable& other) override;
	virtual bool obstructsSight(Player* p) override;
};

struct CollisionMask : public Collideable {
	CollisionMask(const FlatBuffGenerated::CollisionMask*);
	virtual bool obstructsSight(Player*) override { return true; }
};

struct PlayerWall : public Collideable {
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
	std::vector<std::unique_ptr<Object>> objects;
	std::vector<std::unique_ptr<Decoration>> decoration;
	std::vector<std::unique_ptr<CollisionMask>> collisionMasks;
	std::vector<std::unique_ptr<HidingSpot>> hidingspots;
	std::vector<std::unique_ptr<PlayerWall>> playerwalls;
	std::unordered_map<std::string, std::unique_ptr<NavPoint>> navpoints;
	std::vector<std::unique_ptr<Tileinfo>> tileinfo;
	std::unique_ptr<Tilelayer> tilelayer;
	glm::vec2 size;
};

struct MobManipulator {
	b2Vec2 pos;
	bool dispersor;
	uint16_t framesLeft;
};

struct GameState {
private:
	std::mutex mut;

public:
	GamePhase phase = GamePhase::LOBBY;
	std::unique_ptr<Level> level = nullptr;

	std::unique_ptr<b2World> b2world = nullptr;
	std::vector<std::unique_ptr<Player>> players;
	std::map<uint16_t, std::unique_ptr<Civilian>> civilians;
	std::vector<std::unique_ptr<InkParticle>> inkParticles;
	std::vector<MobManipulator> mobManipulators;

	inline std::unique_ptr<std::lock_guard<std::mutex>> lock() {
		return std::make_unique<std::lock_guard<std::mutex>>(mut);
	}

	boost_po::variables_map options;
};

extern GameState gameState;
