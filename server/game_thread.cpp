#include <limits>

#define GLM_ENABLE_EXPERIMENTAL
#include <random>
#include <glm/gtx/vector_angle.hpp>

#include "../common/geometry.hpp"

#include "deadfish.hpp"
#include "game_thread.hpp"
#include "level_loader.hpp"
#include "skills.hpp"

const float GOLDFISH_CHANCE = 0.05f;

uint16_t newMobID()
{
	while (true)
	{
		uint16_t ret = random() % UINT16_MAX;

		for (auto &p : gameState.players)
		{
			if (p->mobID == ret)
				continue;
		}

		auto it = gameState.civilians.find(ret);
		if (it != gameState.civilians.end())
			continue;

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
	for (auto &p : gameState.players)
	{
		dfws::SendData(p->wsHandle, data);
	}
}

Player* getPlayerByConnHdl(dfws::Handle hdl)
{
	auto player = gameState.players.begin();
	while (player != gameState.players.end())
	{
		if ((*player)->wsHandle == hdl)
		{
			return player->get();
		}
		player++;
	}
	std::cout << "getPlayerByConnHdl PLAYER NOT FOUND\n";
	return nullptr;
}

struct FOVCallback
	: public b2RayCastCallback
{
	float32 ReportFixture(b2Fixture *fixture, UNUSED const b2Vec2 &point, UNUSED const b2Vec2 &normal, UNUSED float32 fraction)
	{
		auto data = (Collideable *)fixture->GetBody()->GetUserData();
		// on return 1.f the currently reported fixture will be ignored and the raycast will continue
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
	Mob *target = nullptr;
	Player *player = nullptr;
};

bool playerSeeMob(Player &p, Mob &m)
{
	if (m.isDead())
		return false; // can't see dead ppl lol
	FOVCallback fovCallback;
	fovCallback.target = &m;
	fovCallback.player = &p;
	auto ppos = p.deathTimeout > 0 ? g2b(p.targetPosition) : p.body->GetPosition();
	auto mpos = m.body->GetPosition();
	gameState.b2world->RayCast(&fovCallback, ppos, mpos);
	return fovCallback.closest && fovCallback.closest->GetBody() == m.body;
}

bool mobSeePoint(Mob &m, b2Vec2 &point)
{
	FOVCallback fovCallback;
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
		force, playerSeeMob(rootPlayer, otherPlayer));
}

flatbuffers::Offset<FlatBuffGenerated::Mob> createFBMob(flatbuffers::FlatBufferBuilder &builder,
	Player& player, const Mob* m)
{
	auto distance = b2Distance(m->body->GetPosition(), player.body->GetPosition());
	FlatBuffGenerated::PlayerRelation relation = FlatBuffGenerated::PlayerRelation_None;
	if (player.killTarget == m)
		relation = FlatBuffGenerated::PlayerRelation_Targeted;
	auto posVec = FlatBuffGenerated::Vec2(m->body->GetPosition().x, m->body->GetPosition().y);
	return FlatBuffGenerated::CreateMob(builder,
									m->mobID,
									&posVec,
									m->body->GetAngle(),
									(FlatBuffGenerated::MobState)m->state,
									m->species,
									relation);
}

flatbuffers::Offset<void> makeWorldState(Player &player, flatbuffers::FlatBufferBuilder &builder, uint64_t framesRemaining)
{
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Mob>> mobs;
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Indicator>> indicators;
	for (auto &p : gameState.civilians)
	{
		auto &c = p.second;
		if (!playerSeeMob(player, *c))
			continue;
		auto mob = createFBMob(builder, player, c.get());
		mobs.push_back(mob);
	}
	for (auto &p : gameState.players)
	{
		if (p->deathTimeout > 0)
		{
			// is dead, don't send
			continue;
		}
		bool differentPlayer = p->playerID != player.playerID;
		bool canSeeOther;
		if (differentPlayer)
		{
			canSeeOther = playerSeeMob(player, *p);
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
	for (auto& ink : gameState.inkParticles) {
		FlatBuffGenerated::Vec2 pos = b2f(ink->body->GetPosition());
		auto inkOffset = FlatBuffGenerated::CreateInkParticle(builder, ink->inkID, &pos);
		inkParticles.push_back(inkOffset);
	}

	auto inkParticlesOffset = builder.CreateVector(inkParticles);

	auto worldState = FlatBuffGenerated::CreateWorldState(builder, mobsOffset, indicatorsOffset, inkParticlesOffset, framesRemaining, hidingspot);

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

	c->mobID = newMobID();
	c->species = species;
	c->previousNavpoint = spawnName;
	c->currentNavpoint = spawnName;
	physicsInitMob(c.get(), spawn->position, 0, 0.3f);
	c->setNextNavpoint();
	gameState.civilians[c->mobID] = std::move(c);
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
			float minDist = std::numeric_limits<float>::max();
			for (auto &pl : gameState.players)
			{
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

Mob &findMobById(uint16_t id)
{
	auto it = gameState.civilians.find(id);
	if (it != gameState.civilians.end())
		return *it->second;

	auto it2 = std::find_if(gameState.players.begin(), gameState.players.end(),
							[id](const auto &p) { return p->mobID == id; });
	if (it2 != gameState.players.end())
		return *(*it2);
	std::cout << "could not find mob by id " << id << "\n";
	abort();
}

void executeCommandKill(Player &player, uint16_t id)
{
	if (player.bombsAffecting > 0)
		return;
	player.lastAttack = std::chrono::system_clock::now();
	auto &m = findMobById(id);
	try {
		auto &otherPlayer = dynamic_cast<Player &>(m);
		if (otherPlayer.killTarget && otherPlayer.killTarget->mobID == player.mobID) {
			// they're already targeting us, abort
			return;
		}
	} catch(...) {}
	auto distance = b2Distance(m.body->GetPosition(), player.body->GetPosition());
	if (distance < INSTA_KILL_DISTANCE)
	{
		player.setAttacking();
		m.handleKill(player);
		return;
	}
	player.killTarget = &m;
}

void sendHighscores()
{
	flatbuffers::FlatBufferBuilder builder;
	std::vector<flatbuffers::Offset<FlatBuffGenerated::HighscoreEntry>> entries;
	for (auto &p : gameState.players)
	{
		auto entry = FlatBuffGenerated::CreateHighscoreEntry(builder, p->playerID, p->points);
		entries.push_back(entry);
	}
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
	if (p.skills[skillPos] == (uint16_t) Skills::SKILL_NONE)
		return;
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
		p->killTarget = nullptr;
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

template<typename C>
void updateCollideables(std::vector<std::unique_ptr<C>>& collideables) {
	std::vector<int> despawns;
	for (size_t i = 0; i < collideables.size(); i++)
	{
		collideables[i]->update();
		if (collideables[i]->toBeDeleted)
			despawns.push_back(i);
	}
	for (int i = despawns.size() - 1; i >= 0; i--)
		collideables.erase(collideables.begin() + despawns[i]);
}

void gameThread()
{
	TestContactListener tcl;
	flatbuffers::FlatBufferBuilder builder(1);

	{
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
		if (gameState.options.count("test") == 0)
			std::shuffle(gameState.players.begin(), gameState.players.end(), std::mt19937(std::random_device()()));

		uint8_t lastSpecies = 0;
		for (auto &player : gameState.players)
		{
			player->species = lastSpecies;
			lastSpecies++;
			spawnPlayer(*player);
		}
	}

	int civilianTimer = 0;
	uint64_t roundTimer = ROUND_LENGTH;

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
			exit(0);
		}

		// update physics
		gameState.b2world->Step(1 / 20.0, 8, 3);

		updateCollideables(gameState.inkParticles);

		// update civilians
		for (auto it = gameState.civilians.cbegin(); it != gameState.civilians.cend();) {
			it->second->update();
			if (it->second->toBeDeleted)
				it = gameState.civilians.erase(it);
			else
				++it;
		}

		// update players
		for (auto &p : gameState.players)
			p->update();

		// spawn civilians if need be
		if (!gameState.options["ghosttown"].as<bool>() && civilianTimer == 0 && gameState.civilians.size() < MAX_CIVILIANS)
		{
			spawnCivilians();
			civilianTimer = CIVILIAN_TIME;
		}
		else
			civilianTimer = std::max(0, civilianTimer - 1);

		// send data to everyone
		for (auto &p : gameState.players)
		{
			builder.Clear();
			auto offset = makeWorldState(*p, builder, roundTimer);
			sendServerMessage(*p, builder, FlatBuffGenerated::ServerMessageUnion_WorldState, offset);
		}

		// Drop the lock
		maybe_guard.reset();

		// sleep for the remaining of time
		std::this_thread::sleep_until(frameStart + std::chrono::milliseconds(FRAME_TIME));
	}
}
