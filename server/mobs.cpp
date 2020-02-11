#include "deadfish.hpp"
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/random.hpp>
#include <iostream>
#include "game_thread.hpp"

const float TURN_SPEED = 4.f;
const float WALK_SPEED = 1.f;
const float RUN_SPEED = 2.f;
const float CLOSE = 0.05f;
const float ANGULAR_CLOSE = 0.1f;
const float TARGET_OFFSET = 0.3f;
const float RANDOM_OFFSET = 0.15f;
const int DEATH_TIMEOUT = 20 * 2;
const float TOO_SLOW = 0.4f;
const int CIV_SLOW_FRAMES = 40;
const int CIV_RESET_FRAMES = 80;

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
    auto speed = this->state == MobState::WALKING ? WALK_SPEED : RUN_SPEED;
    auto translation = glm::rotate(glm::vec2(1, 0), this->body->GetAngle() - (float)(M_PI / 2)) * speed;
    this->body->SetLinearVelocity(b2Vec2(translation.x, translation.y));
}

void Player::reset()
{
    gameState.b2world->DestroyBody(this->body);
    this->killTarget = nullptr;
    this->toBeDeleted = false;
    this->state = MobState::WALKING;
    this->lastAttack = std::chrono::system_clock::from_time_t(0);
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
    }
    if (this->toBeDeleted)
    {
        // kill player
        this->targetPosition = b2g(this->body->GetPosition());
        this->reset();
        this->deathTimeout = DEATH_TIMEOUT;
    }
    if (this->killTarget)
    {
        this->targetPosition = b2g(this->killTarget->body->GetPosition());
    }
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
        // std::cout << std::get<0>(p) << " " << std::get<1>(p) << " " << std::get<2>(p) << "\n";
        auto spawnName = std::get<1>(p);
        auto& navpoint = gameState.level->navpoints[spawnName];
        auto spawnPos = g2b(navpoint->position);

        // not where we were currently going but somewhere we can go immediately from here
        if (this->currentNavpoint != spawnName && mobSeePoint(*this, spawnPos)) {
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
    auto speed = b2Distance(this->body->GetPosition(), this->lastPos);
    if (b2Distance(this->body->GetPosition(), this->lastPos) < (WALK_SPEED/20)*0.4f) {
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
    try
    {
        auto &mob = dynamic_cast<Mob &>(other);

        if (this->killTarget && this->killTarget->id == mob.id)
        {
            // the player wants to kill the mob and collided with him, execute the kill
            executeKill(*this, mob);
        }
    }
    catch (...)
    {
    }
}

Mob::~Mob()
{
    if (this->body)
    {
        gameState.b2world->DestroyBody(this->body);
        this->body = nullptr;
    }
}