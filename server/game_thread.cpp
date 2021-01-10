#include <limits>

#define GLM_ENABLE_EXPERIMENTAL
#include <random>
#include <glm/gtx/vector_angle.hpp>

#include "../common/geometry.hpp"

#include "deadfish.hpp"
#include "game_thread.hpp"
#include "level_loader.hpp"
#include "skills.hpp"
#include "agones.hpp"

const float GOLDFISH_CHANCE = 0.05f;
const uint32_t PRESIMULATE_TICKS = 1000;

uint16_t newMovableID()
{
	while (true)
	{
		uint16_t ret = random() % UINT16_MAX;

		if (ret == 0)
		continue;

		{
			auto it = gameState.players.find(ret);
			if (it != gameState.players.end())
				continue;
		}

		{
			auto it = gameState.civilians.find(ret);
			if (it != gameState.civilians.end())
				continue;
		}

		{
			auto it = gameState.inkParticles.find(ret);
			if (it != gameState.inkParticles.end())
				continue;
		}

		{		
			auto it = gameState.mobManipulators.find(ret);
			if (it != gameState.mobManipulators.end())
				continue;
		}

		return ret;
	}
}

std::string makeServerMessage(flatbuffers::FlatBufferBuilder &builder,
							  FlatBuffGenerated::ServerMessageUnion type,
							  flatbuffers::Offset<void> offset)
{
	auto message = FlatBuffGenerated::CreateServerMessage(builder,
							type,
							offset);
	builder.Finish(message);
	auto data = builder.GetBufferPointer();
	auto size = builder.GetSize();
	auto str = std::string(data, data + size);
	return str;
}

void sendGameAlreadyInProgress(dfws::Handle hdl)
{
	std::cout << "sending game already in progress";
	flatbuffers::FlatBufferBuilder builder;
	auto offset = FlatBuffGenerated::CreateSimpleServerEvent(builder,
		FlatBuffGenerated::SimpleServerEventType_GameAlreadyInProgress);
	auto str = makeServerMessage(builder, FlatBuffGenerated::ServerMessageUnion_SimpleServerEvent,
		offset.Union());
	dfws::SendData(hdl, str);
}

void sendServerMessage(Player &player,
					   flatbuffers::FlatBufferBuilder &builder,
					   FlatBuffGenerated::ServerMessageUnion type,
					   flatbuffers::Offset<void> offset)
{
	auto str = makeServerMessage(builder, type, offset);
	dfws::SendData(player.wsHandle, str);
}

void sendToAll(std::string &data)
{
	iterateOverMovableMap(gameState.players,
		std::function<void(Player&)>([=](Player& p){
			dfws::SendData(p.wsHandle, data);
		})
	);
}

Player* getPlayerByConnHdl(dfws::Handle hdl)
{
	Player* ret = nullptr;
	iterateOverMovableMap(gameState.players,
		std::function<void(Player&)>([&](Player& p){
			if (p.wsHandle == hdl)
				ret = &p;
		})
	);
	if (!ret)
		std::cout << "getPlayerByConnHdl PLAYER NOT FOUND\n";
	return ret;
}

// void updateCollideableMovable(CollideableMovable* cm)
// {
// 	cm->pos = b2f(cm->body->GetPosition());
// 	cm->angle = cm->body->GetAngle();
// }

// void iterateOverCollideableMovable(std::function<void(CollideableMovable&)> f)
// {

// }

struct FOVCallback
	: public b2RayCastCallback
{
	float32 ReportFixture(b2Fixture *fixture, UNUSED const b2Vec2 &point, UNUSED const b2Vec2 &normal, UNUSED float32 fraction)
	{
		auto data = (Collideable *)fixture->GetBody()->GetUserData();
		// on return 1.f the currently reported fixture will be ignored and the raycast will continue
		if (ignoreMobs && dynamic_cast<Mob*>(data))
			return 1.f;

		if (data && data != target && player && !data->obstructsSight(player))
			return 1.f;
		if (fraction < minfraction)
		{
			minfraction = fraction;
			closest = fixture;
		}
		return fraction;
	}
	float minfraction = 1.f;
	b2Fixture *closest = nullptr;
	Collideable *target = nullptr;
	Player *player = nullptr;
	bool ignoreMobs = false;
};

bool playerSeeCollideable(Player &p, Collideable &c)
{
	FOVCallback fovCallback;
	fovCallback.target = &c;
	fovCallback.player = &p;
	auto ppos = p.deathTimeout > 0 ? g2b(p.targetPosition) : p.body->GetPosition();
	auto cpos = c.body->GetPosition();
	gameState.b2world->RayCast(&fovCallback, ppos, cpos);
	return fovCallback.closest && fovCallback.closest->GetBody() == c.body;
}

bool mobSeePoint(Mob &m, const b2Vec2 &point, bool ignoreMobs)
{
	if (b2Distance(m.body->GetPosition(), point) == 0.0f)
		return true;
	FOVCallback fovCallback;
	fovCallback.ignoreMobs = ignoreMobs;
	gameState.b2world->RayCast(&fovCallback, m.body->GetPosition(), point);
	return fovCallback.minfraction == 1.f;
}

float revLerp(float min, float max, float val)
{
	if (val < min)
		return 0;
	if (val > max)
		return 1;
	return (val - min) / (max - min);
}

flatbuffers::Offset<FlatBuffGenerated::Indicator>
makePlayerIndicator(flatbuffers::FlatBufferBuilder &builder,
					Player &rootPlayer,
					Player &otherPlayer)
{
	glm::vec2 toTarget = b2g(otherPlayer.body->GetPosition()) - b2g(rootPlayer.body->GetPosition());
	float force = 1.f - revLerp(6, 12, glm::length(toTarget));
	float angle = 0;
	if (force != 0)
		angle = angleFromVector(toTarget);
	return FlatBuffGenerated::CreateIndicator(builder, angle,
		force, playerSeeCollideable(rootPlayer, otherPlayer));
}

flatbuffers::Offset<FlatBuffGenerated::Mob> createFBMob(flatbuffers::FlatBufferBuilder &builder,
	Player& player, const Mob* m)
{
	FlatBuffGenerated::PlayerRelation relation = FlatBuffGenerated::PlayerRelation_None;
	if (player.killTargetID == m->movableID)
		relation = FlatBuffGenerated::PlayerRelation_Targeted;
	auto posVec = FlatBuffGenerated::Vec2(m->body->GetPosition().x, m->body->GetPosition().y);
	// TODO(movable)
	// return FlatBuffGenerated::CreateMob(builder,
	// 								m->movableID,
	// 								&posVec,
	// 								m->body->GetAngle(),
	// 								(FlatBuffGenerated::MobState)m->state,
	// 								m->species,
	// 								relation);
}

flatbuffers::Offset<void> makeWorldState(Player &player, flatbuffers::FlatBufferBuilder &builder, uint64_t framesRemaining)
{
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Mob>> mobs;
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Indicator>> indicators;
	// TODO(movable) refactor this so that all the movables are processed together
	for (auto &p : gameState.civilians)
	{
		auto &c = p.second;
		if (!playerSeeCollideable(player, *c))
			continue;
		auto mob = createFBMob(builder, player, c.get());
		mobs.push_back(mob);
	}
	for (auto &pIt : gameState.players)
	{
		auto &p = pIt.second;
		if (p->deathTimeout > 0)
		{
			// is dead, don't send
			continue;
		}
		bool differentPlayer = p->playerID != player.playerID;
		bool canSeeOther;
		if (differentPlayer)
		{
			canSeeOther = playerSeeCollideable(player, *p);
			auto indicator = makePlayerIndicator(builder, player, *p);
			indicators.push_back(indicator);
		}
		if (differentPlayer && !canSeeOther)
			continue;
		auto mob = createFBMob(builder, player, p.get());
		mobs.push_back(mob);
	}
	std::string hspotname = ""; // name of the hidingspot that the player is in
	for(auto &hspot : gameState.level->hidingspots) {
		auto playerInHspot = hspot->playersInside.find(&player);
		if (playerInHspot != hspot->playersInside.end()) {
			hspotname = hspot->name;
			break;
		}
	}
	auto mobsOffset = builder.CreateVector(mobs);
	auto indicatorsOffset = builder.CreateVector(indicators);
	auto hidingspot = builder.CreateString(hspotname);

	std::vector<flatbuffers::Offset<FlatBuffGenerated::InkParticle>> inkParticles;
	std::vector<FlatBuffGenerated::Vec2> inkVecs;
	for (auto& inkIt : gameState.inkParticles) {
		auto &ink = inkIt.second;
		if (!playerSeeCollideable(player, *ink))
			continue;
		FlatBuffGenerated::Vec2 pos = b2f(ink->body->GetPosition());
		// TODO(movable)
		// auto inkOffset = FlatBuffGenerated::CreateInkParticle(builder, ink->movableID, &pos);
		// inkParticles.push_back(inkOffset);
	}

	auto inkParticlesOffset = builder.CreateVector(inkParticles);

	std::vector<flatbuffers::Offset<FlatBuffGenerated::MobManipulator>> manipulators;
	for (auto &manipulatorIt : gameState.mobManipulators) {
		auto &manipulator = manipulatorIt.second;
		if (!mobSeePoint(player, f2b(manipulator->pos), true))
			continue;
		// TODO(movable)
		// auto manOffset = FlatBuffGenerated::CreateMobManipulator(builder, &manipulator.pos, manipulator.type);
		// manipulators.push_back(manOffset);
	}

	auto manipulatorsOffset = builder.CreateVector(manipulators);

	auto worldState = FlatBuffGenerated::CreateWorldState(builder, mobsOffset, indicatorsOffset, inkParticlesOffset, framesRemaining, manipulatorsOffset, hidingspot);

	return worldState.Union();
}

class TestContactListener : public b2ContactListener
{
	void BeginContact(b2Contact *contact) override
	{
		auto collideableA = (Collideable *)contact->GetFixtureA()->GetBody()->GetUserData();
		auto collideableB = (Collideable *)contact->GetFixtureB()->GetBody()->GetUserData();

		if (collideableA && !collideableA->toBeDeleted &&
			collideableB && !collideableB->toBeDeleted)
		{
			collideableA->handleCollision(*collideableB);
			collideableB->handleCollision(*collideableA);
		}
	}

	void EndContact(b2Contact *contact) override
	{
		auto collideableA = (Collideable *)contact->GetFixtureA()->GetBody()->GetUserData();
		auto collideableB = (Collideable *)contact->GetFixtureB()->GetBody()->GetUserData();

		if (collideableA && !collideableA->toBeDeleted &&
			collideableB && !collideableB->toBeDeleted)
		{
			collideableA->endCollision(*collideableB);
			collideableB->endCollision(*collideableA);
		}
	}
};

void physicsInitMob(Mob *m, glm::vec2 pos, float angle, float radius, uint16 categoryBits = 1)
{
	b2BodyDef myBodyDef;
	myBodyDef.type = b2_dynamicBody;      //this will be a dynamic body
	myBodyDef.position.Set(pos.x, pos.y); //set the starting position
	myBodyDef.angle = angle;              //set the starting angle
	m->body = gameState.b2world->CreateBody(&myBodyDef);
	b2CircleShape circleShape;
	circleShape.m_radius = radius;

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circleShape;
	fixtureDef.density = 1;
	fixtureDef.friction = 0;
	fixtureDef.filter.categoryBits = categoryBits;
	fixtureDef.filter.maskBits = 0xFFFF;
	m->body->CreateFixture(&fixtureDef);
	m->body->SetUserData(m);
}

std::vector<int> civiliansSpeciesCount()
{
	std::vector<int> ret;
	ret.resize(gameState.players.size());
	for (auto &p : gameState.civilians)
	{
		auto &c = p.second;
		if (c->species != GOLDFISH_SPECIES)
			ret[c->species]++;
	}
	return ret;
}

void spawnCivilian(std::string spawnName, NavPoint* spawn) {
	auto c = std::make_unique<Civilian>();

	int species = 0;

	float goldfishBet = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	if (goldfishBet < GOLDFISH_CHANCE)
		species = GOLDFISH_SPECIES;
	else {
		// find species with lowest count of civilians
		auto civCounts = civiliansSpeciesCount();
		int lowestSpecies = 0;
		int lowestSpeciesCount = INT_MAX;
		for (size_t i = 0; i < civCounts.size(); i++)
		{
			if (civCounts[i] < lowestSpeciesCount)
			{
				lowestSpecies = i;
				lowestSpeciesCount = civCounts[i];
			}
		}
		species = lowestSpecies;
	}

	c->movableID = newMovableID();
	c->species = species;
	c->previousNavpoint = spawnName;
	c->currentNavpoint = spawnName;
	physicsInitMob(c.get(), spawn->position, 0, 0.3f);
	c->setNextNavpoint();
	gameState.civilians[c->movableID] = std::move(c);
	std::cout << "spawning civilian of species " << species <<
		" at " << spawnName << " to a total of " << gameState.civilians.size() << "\n";
}

void spawnCivilians()
{
	// spawn on all spawnpoints
	std::vector<std::string> spawns;
	for (auto &p : gameState.level->navpoints)
	{
		if (p.second->isspawn)
		{
			spawnCivilian(p.first, p.second.get());
		}
	}
}

void spawnPlayer(Player &player)
{
	// find spawns
	uint64_t maxMinDist = 0;
	std::string maxSpawn = "";
	for (auto &p : gameState.level->navpoints)
	{
		if (p.second->isplayerspawn)
		{
			// TODO(movable) iterate over players reasonably
			float minDist = std::numeric_limits<float>::max();
			for (auto &plIt : gameState.players)
			{
				auto &pl = plIt.second;
				if (!pl->body)
					continue;
				auto dist = b2Distance(g2b(p.second->position), pl->body->GetPosition());
				if (dist < minDist)
				{
					minDist = dist;
				}
			}
			if (minDist > maxMinDist)
			{
				maxMinDist = minDist;
				maxSpawn = p.first;
			}
		}
	}
	auto spawn = gameState.level->navpoints[maxSpawn].get();
	if (!spawn) {
		std::cout << "could not find any spawn points on the map\n";
		exit(1);
	}
	physicsInitMob(&player, spawn->position, 0, 0.3f, 3);
	player.targetPosition = spawn->position;

	std::cout << "spawned player at " << maxSpawn << ", " << player.body->GetPosition() << "\n";
}

Mob *findMobById(uint16_t id)
{
	{
		auto it = gameState.civilians.find(id);
		if (it != gameState.civilians.end())
			return it->second.get();
	}

	{
		auto it = gameState.players.find(id);
		if (it != gameState.players.end())
			return it->second.get();
	}

	return nullptr;
}

void executeCommandKill(Player &player, uint16_t id)
{
	if (player.bombsAffecting > 0)
		return;
	player.lastAttack = std::chrono::system_clock::now();
	auto m = findMobById(id);
	if (!m)
		return;
	auto otherPlayer = dynamic_cast<Player *>(m);
	if (otherPlayer && otherPlayer->killTargetID == player.movableID) {
		// they're already targeting us, abort
		return;
	}
	auto distance = b2Distance(m->body->GetPosition(), player.body->GetPosition());
	if (distance < INSTA_KILL_DISTANCE)
	{
		player.setAttacking();
		m->handleKill(player);
		return;
	}
	player.killTargetID = m->movableID;
}

void sendHighscores()
{
	flatbuffers::FlatBufferBuilder builder;
	std::vector<flatbuffers::Offset<FlatBuffGenerated::HighscoreEntry>> entries;
	iterateOverMovableMap(gameState.players,
		std::function<void(Player&)>([&](Player& p){
			auto entry = FlatBuffGenerated::CreateHighscoreEntry(builder, p.playerID, p.points);
			entries.push_back(entry);
		})
	);
	auto v = builder.CreateVector(entries);
	auto update = FlatBuffGenerated::CreateHighscoreUpdate(builder, v);
	auto data = makeServerMessage(builder, FlatBuffGenerated::ServerMessageUnion_HighscoreUpdate, update.Union());
	sendToAll(data);
}

void executeSkill(Player& p, uint8_t skillPos, b2Vec2 mousePos) {
	if (skillPos >= p.skills.size()) {
		std::cout << "skillPos out of bounds\n";
		return;
	}
	Skills skill = (Skills) p.skills[skillPos];
	auto skillHandler = skillHandlers[(uint16_t) skill];
	if (skillHandler == nullptr) {
		std::cout << "no such skill handler " << (uint16_t) skill << "\n";
	}
	bool used = skillHandler(p, skill, mousePos);
	if (used) {
		p.skills.erase(p.skills.begin() + skillPos);
		p.sendSkillBarUpdate();
	}
}

void gameOnMessage(dfws::Handle hdl, const std::string& payload)
{
	const auto clientMessage = flatbuffers::GetRoot<FlatBuffGenerated::ClientMessage>(payload.c_str());
	const auto guard = gameState.lock();

	auto p = getPlayerByConnHdl(hdl);
	if (!p) {
		sendGameAlreadyInProgress(hdl);
		return;
	}
	if (p->state == MobState::ATTACKING)
		return;

	switch (clientMessage->event_type())
	{
	case FlatBuffGenerated::ClientMessageUnion::ClientMessageUnion_CommandMove:
	{
		const auto event = clientMessage->event_as_CommandMove();
		p->targetPosition = glm::vec2(event->target()->x(), event->target()->y());
		p->state = p->state == MobState::RUNNING ? MobState::RUNNING : MobState::WALKING;
		p->killTargetID = 0;
		p->lastAttack = std::chrono::system_clock::from_time_t(0);
	}
	break;
	case FlatBuffGenerated::ClientMessageUnion::ClientMessageUnion_CommandRun:
	{
		const auto event = clientMessage->event_as_CommandRun();
		p->state = event->run() ? MobState::RUNNING : MobState::WALKING;
	}
	break;
	case FlatBuffGenerated::ClientMessageUnion::ClientMessageUnion_CommandKill:
	{
		const auto event = clientMessage->event_as_CommandKill();
		executeCommandKill(*p, event->mobID());
	}
	break;
	case FlatBuffGenerated::ClientMessageUnion::ClientMessageUnion_CommandSkill:
	{
		const auto event = clientMessage->event_as_CommandSkill();
		executeSkill(*p, event->skill(), {event->mousePos()->x(), event->mousePos()->y()});
	}
	break;

	default:
		std::cout << "gameOnMessage: some other message type received\n";
		break;
	}
}

void initGameThread(TestContactListener& tcl)
{
	flatbuffers::FlatBufferBuilder builder(1);
	const auto guard = gameState.lock();

	// init physics
	gameState.b2world = std::make_unique<b2World>(b2Vec2(0, 0));
	gameState.b2world->SetContactListener(&tcl);

	// load level
	gameState.level = std::make_unique<Level>();
	auto path = gameState.options["level"].as<std::string>();
	loadLevel(path);

	// send level to clients
	auto levelOffset = serializeLevel(builder);
	auto data = makeServerMessage(builder, FlatBuffGenerated::ServerMessageUnion_Level, levelOffset.Union());
	sendToAll(data);

	// shuffle the players to give them random species unless it's a test
	// TODO(moveable) somehow shuffle a map lmao
	// if (gameState.options.count("test") == 0)
	// 	std::shuffle(gameState.players.begin(), gameState.players.end(), std::mt19937(std::random_device()()));

	uint8_t lastSpecies = 0;
	iterateOverMovableMap(gameState.players,
		std::function<void(Player&)>([&](Player& p){
			p.species = lastSpecies;
			lastSpecies++;
			spawnPlayer(p);
		})
	);
}

void gameThreadTick(int& civilianTimer)
{
	// update physics
	gameState.b2world->Step(1 / 20.0, 8, 3);

	// TODO(moveable) rewrite updating with new design
	// updateCollideables(gameState.inkParticles);

	// update civilians
	for (auto it = gameState.civilians.cbegin(); it != gameState.civilians.cend();) {
		it->second->update();
		if (it->second->toBeDeleted)
			it = gameState.civilians.erase(it);
		else
			++it;
	}

	// update players
	// TODO(moveable) rewrite updating with new design
	// for (auto &p : gameState.players)
	// 	p->update();

	// spawn civilians if need be
	if (!gameState.options["ghosttown"].as<bool>() && civilianTimer == 0 && gameState.civilians.size() < MAX_CIVILIANS)
	{
		spawnCivilians();
		civilianTimer = CIVILIAN_TIME;
	}
	else
		civilianTimer = std::max(0, civilianTimer - 1);

	// TODO(moveable) rewrite updating with new design
	// for (auto it = gameState.mobManipulators.begin(); it != gameState.mobManipulators.end();) {
	// 	it->framesLeft--;
	// 	if (it->framesLeft == 0)
	// 		it = gameState.mobManipulators.erase(it);
	// 	else
	// 		it++;
	// }
}

void gameThread()
{
	TestContactListener tcl;
	flatbuffers::FlatBufferBuilder builder(1);

	int civilianTimer = 0;
	uint64_t roundTimer = ROUND_LENGTH;

	initGameThread(tcl);

	for (uint32_t i = 0; i < PRESIMULATE_TICKS; i++) {
		gameThreadTick(civilianTimer);
	}

	// game loop
	while (true)
	{
		auto maybe_guard = gameState.lock();
		auto frameStart = std::chrono::system_clock::now();

		roundTimer--;
		if (roundTimer == 0)
		{
			// send game end message to everyone
			builder.Clear();
			auto ev = FlatBuffGenerated::CreateSimpleServerEvent(builder, FlatBuffGenerated::SimpleServerEventType_GameEnded);
			auto data = makeServerMessage(builder, FlatBuffGenerated::ServerMessageUnion_SimpleServerEvent, ev.Union());
			sendToAll(data);
			// FIXME: Proper closing of all connections, so that this sleep is unnecessary
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			agones::Shutdown();
			return;
		}

		gameThreadTick(civilianTimer);

		iterateOverMovableMap(gameState.players,
			std::function<void(Player&)>([&](Player& p){
				builder.Clear();
				auto offset = makeWorldState(p, builder, roundTimer);
				sendServerMessage(p, builder, FlatBuffGenerated::ServerMessageUnion_WorldState, offset);
			})
		);

		// Drop the lock
		maybe_guard.reset();

		// sleep for the remaining of time
		std::this_thread::sleep_until(frameStart + std::chrono::milliseconds(FRAME_TIME));
	}
}
