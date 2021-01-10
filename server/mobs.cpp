#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/random.hpp>

#include "deadfish.hpp"
#include "game_thread.hpp"
#include "../common/geometry.hpp"

std::ostream &operator<<(std::ostream &os, glm::vec2 &v)
{
	os << v.x << "," << v.y;
	return os;
}

std::ostream &operator<<(std::ostream &os, b2Vec2 v)
{
	os << v.x << "," << v.y;
	return os;
}

float Mob::calculateSpeed() {
	if (this->bombsAffecting > 0)
		return WALK_SPEED * INK_BOMB_SPEED_MODIFIER;
	return WALK_SPEED;
}

void Mob::update()
{
	float dist = glm::distance(b2g(this->body->GetPosition()), this->targetPosition);
	if (dist < CLOSE)
	{
		this->body->SetAngularVelocity(0);
		this->body->SetLinearVelocity(b2Vec2(0, 0));
		return; // don't move if really close to target
	}

	// update angle
	glm::vec2 toTarget = glm::normalize(this->targetPosition - b2g(this->body->GetPosition()));
	float currentAngle = -glm::orientedAngle(toTarget, glm::vec2(1, 0)) + M_PI / 2;
	float diff = currentAngle - this->body->GetAngle();

	if (diff < -M_PI)
		diff += 2 * M_PI;
	if (diff > M_PI)
		diff -= 2 * M_PI;

	bool skip = false;
	auto turnSpeed = TURN_SPEED;
	if (abs(diff) < ANGULAR_CLOSE * 2)
	{
		turnSpeed /= 2.;
	}
	if (abs(diff) < ANGULAR_CLOSE)
	{
		this->body->SetAngularVelocity(0);
		skip = true;
	}
	if (!skip)
	{
		if (diff > 0)
		{
			this->body->SetAngularVelocity(turnSpeed);
		}
		else
		{
			this->body->SetAngularVelocity(-turnSpeed);
		}
	}

	// update position
	auto translation = glm::rotate(glm::vec2(1, 0), this->body->GetAngle() - (float)(M_PI / 2)) * this->calculateSpeed();
	this->body->SetLinearVelocity(b2Vec2(translation.x, translation.y));
}

float Player::calculateSpeed() {
	if (this->bombsAffecting > 0)
		return WALK_SPEED * INK_BOMB_SPEED_MODIFIER;
	float speed = WALK_SPEED;
	if (this->state == MobState::RUNNING)
		speed *= RUN_MODIFIER;
	if (this->killTargetID != 0)
		speed *= CHASE_SPEED_BONUS;
	return speed;
}

void Player::reset()
{
	gameState.b2world->DestroyBody(this->body);
	for(auto& hspot : gameState.level->hidingspots) {
		hspot->playersInside.erase(this);
	}
	this->killTargetID = 0;
	this->toBeDeleted = false;
	this->state = MobState::WALKING;
	this->lastAttack = std::chrono::system_clock::from_time_t(0);
	this->bombsAffecting = 0;
	this->skills.clear();
	this->sendSkillBarUpdate();
}

void Player::update()
{
	if (this->deathTimeout > 0)
	{
		this->deathTimeout--;
		if (this->deathTimeout == 0)
			spawnPlayer(*this);
		return;
	}
	if (this->attackTimeout > 0)
	{
		this->attackTimeout--;
		if (this->attackTimeout == 0)
		{
			this->state = MobState::WALKING;
			this->lastAttack = std::chrono::system_clock::from_time_t(0);
		}
		this->body->SetAngularVelocity(0);
		this->body->SetLinearVelocity({0, 0});
		return; // disable dying while killing and moving while killing
	}
	if (this->toBeDeleted)
	{
		// kill player
		this->targetPosition = b2g(this->body->GetPosition());
		this->reset();
		this->deathTimeout = DEATH_TIMEOUT;
	}
	if (this->bombsAffecting > 0)
		this->killTargetID = 0;

	auto killTarget = findMobById(this->killTargetID);
	if (killTarget && !playerSeeCollideable(*this, *killTarget)) {
		this->killTargetID = 0;
		this->targetPosition = b2g(this->body->GetPosition());
	}

	killTarget = findMobById(this->killTargetID);
	if (killTarget)
		this->targetPosition = b2g(killTarget->body->GetPosition());
	Mob::update();
}

glm::vec2 randFromCircle(glm::vec2 center, float radius)
{
	float x = (center.x - radius) + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2 * radius)));
	float y = (center.y - radius) + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2 * radius)));
	glm::vec2 ret = {x, y};
	if (glm::distance(ret, center) <= radius)
		return ret; 
	return randFromCircle(center, radius);
}

void Civilian::collisionResolution() {
	std::vector<std::tuple<float, std::string>> navpoints;
	for (auto& n : gameState.level->navpoints) {
		auto bpos = g2b(n.second->position);
		auto dist = b2Distance(this->body->GetPosition(), bpos);
		navpoints.push_back({
			dist,
			n.first
		});
	}
	std::sort(navpoints.begin(), navpoints.end());
	for (auto& p : navpoints) {
		auto spawnName = std::get<1>(p);
		auto& navpoint = gameState.level->navpoints[spawnName];
		auto spawnPos = g2b(navpoint->position);

		// not where we were currently going but somewhere we can go immediately from here
		if (this->currentNavpoint != spawnName && mobSeePoint(*this, spawnPos, true)) {
			this->previousNavpoint = "";
			this->targetPosition = randFromCircle(navpoint->position, navpoint->radius);
			std::cout << "resolved collision - changed direction\n";
			return;
		}
	}
	std::cout << "could not resolve collision - DESPAWN\n";
	this->toBeDeleted = true;
}

void Civilian::update()
{
	std::vector<MobManipulator> seenManips;
	for (auto &m : gameState.mobManipulators) {
		if (mobSeePoint(*this, f2b(m.pos), true))
			seenManips.push_back(m);
	}
	if (seenManips.size() > 0) {
		this->seenAManip = true;
		auto last = seenManips.back();
		if (last.type == FlatBuffGenerated::MobManipulatorType_Attractor)
			this->targetPosition = f2g(last.pos);
		else {
			// dispersor
			auto toManip = this->body->GetPosition() - f2b(last.pos);
			toManip.Normalize();
			this->targetPosition = b2g(this->body->GetPosition() + toManip);
		}
		Mob::update();
		return;
	} else if (this->seenAManip) {
		this->collisionResolution();
		this->seenAManip = false;
	}

	if (this->bombsAffecting == 0 && b2Distance(this->body->GetPosition(), this->lastPos) < (WALK_SPEED/20) * 0.4f) {
		slowFrames++;
		if (slowFrames == CIV_SLOW_FRAMES) {
			this->collisionResolution();
			return;
		}
		if (slowFrames == CIV_RESET_FRAMES) {
			this->toBeDeleted = true;
			std::cout << "collision resolution DELETED CIVILIAN\n";
		}
	} else
		slowFrames = 0;
	this->lastPos = this->body->GetPosition();
	float dist = glm::distance(b2g(this->body->GetPosition()), this->targetPosition);
	if (dist < CLOSE)
	{
		// the civilian reached his destination
		if (gameState.level->navpoints[this->currentNavpoint]->isspawn)
		{
			// we arrived at spawn, despawn
			this->toBeDeleted = true;
			return;
		}
		this->setNextNavpoint();
	}
	Mob::update();
}

void Civilian::setNextNavpoint()
{
	auto &spawn = gameState.level->navpoints[this->currentNavpoint];
	auto neighbors = spawn->neighbors;
	std::vector<std::string>::iterator toDelete = neighbors.end();
	for (auto it = neighbors.begin(); it != neighbors.end(); it++)
	{
		if (*it == this->previousNavpoint)
		{
			toDelete = it;
			break;
		}
	}
	if (toDelete != neighbors.end())
		neighbors.erase(toDelete);
	this->previousNavpoint = this->currentNavpoint;
	this->currentNavpoint = neighbors[rand() % neighbors.size()];
	auto& targetPoint = gameState.level->navpoints[this->currentNavpoint];
	this->targetPosition = randFromCircle(targetPoint->position, targetPoint->radius);
}

void Player::handleCollision(Collideable &other)
{
	if (this->toBeDeleted)
		return;
	
	try
	{
		auto &mob = dynamic_cast<Mob &>(other);

		if (this->killTargetID == mob.movableID)
		{
			this->setAttacking();
			// the player wants to kill the mob and collided with him, execute the kill
			mob.handleKill(*this);
		}
	}
	catch (...)
	{
	}
}

void Player::setAttacking() {
	this->killTargetID = 0;
	this->state = MobState::ATTACKING;
	this->attackTimeout = 40;
}

Mob::~Mob()
{
	if (this->body)
	{
		gameState.b2world->DestroyBody(this->body);
		this->body = nullptr;
	}
}

void handleGoldfishKill(Player& killer) {
	if (killer.skills.size() == MAX_SKILLS)
		return;
	uint16_t skill = rand() % (uint16_t) Skills::SKILLS_MAX;
	killer.skills.push_back(skill);
	killer.sendSkillBarUpdate();
}

void Civilian::handleKill(Player& killer) {
	if (killer.isDead())
		return;
	this->toBeDeleted = true;
	if (this->species == GOLDFISH_SPECIES) {
		handleGoldfishKill(killer);
		return;
	}
	killer.points += CIVILIAN_PENALTY;

	// send the deathreport killed npc message
	flatbuffers::FlatBufferBuilder builder;
	auto ev = FlatBuffGenerated::CreateDeathReport(builder, killer.playerID, -1);
	sendServerMessage(killer, builder, FlatBuffGenerated::ServerMessageUnion_DeathReport, ev.Union());

	sendHighscores();
}

void Player::handleKill(Player& killer) {
	if (killer.isDead())
		return;

	this->toBeDeleted = true;
	killer.points += KILL_REWARD;

	// send the deathreport message
	flatbuffers::FlatBufferBuilder builder;
	auto ev = FlatBuffGenerated::CreateDeathReport(builder, killer.playerID, this->playerID);
	auto data = makeServerMessage(builder, FlatBuffGenerated::ServerMessageUnion_DeathReport, ev.Union());
	sendToAll(data);

	sendHighscores();
}

bool Player::isDead() {
	return this->deathTimeout > 0;
}

void Player::sendSkillBarUpdate() {
	flatbuffers::FlatBufferBuilder builder;
	auto skills = builder.CreateVector(this->skills);
	auto ev = FlatBuffGenerated::CreateSkillBarUpdate(builder, skills);
	sendServerMessage(*this, builder, FlatBuffGenerated::ServerMessageUnion_SkillBarUpdate, ev.Union());
	std::cout << "updated skills of player " << this->name << "\n";
}
