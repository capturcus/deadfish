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

bool Mob::update()
{
    float dist = glm::distance(b2g(this->body->GetPosition()), this->targetPosition);
    if (dist < CLOSE)
    {
        this->body->SetAngularVelocity(0);
        this->body->SetLinearVelocity(b2Vec2(0, 0));
        return true; // don't move if really close to target
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

        // if (this->angle > M_PI)
        //     this->angle -= 2 * M_PI;
        // if (this->angle < M_PI)
        //     this->angle += 2 * M_PI;
    }

    // update position
    auto speed = this->state == MobState::WALKING ? WALK_SPEED : RUN_SPEED;
    auto translation = glm::rotate(glm::vec2(1, 0), this->body->GetAngle() - (float)(M_PI / 2)) * speed;
    this->body->SetLinearVelocity(b2Vec2(translation.x, translation.y));

    // update box
    // this->body->SetTransform(b2Vec2(this->position.x, this->position.y), this->angle);
    // std::cout << "physics pos \t" << this->body->GetPosition().x << "\t" << this->body->GetPosition().y << "\n";
    return true;
}

bool Player::update()
{
    if (this->killTarget) {
        this->targetPosition = b2g(this->killTarget->body->GetPosition());
    }
    return Mob::update();
}

bool Civilian::update()
{
    float dist = glm::distance(b2g(this->body->GetPosition()), this->targetPosition);
    if (dist < CLOSE) {
        // the civilian reached his destination
        if (gameState.level->navpoints[this->currentNavpoint]->isspawn) {
            // we arrived at spawn, despawn
            return false;
        }
        this->setNextNavpoint();
    }
    return Mob::update();
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
    auto targetPoint = gameState.level->navpoints[this->currentNavpoint]->position;
    auto pos = b2g(this->body->GetPosition());
    auto offset = glm::rotate(glm::normalize(targetPoint - pos), glm::pi<float>()/2)*TARGET_OFFSET;
    offset += glm::linearRand(glm::vec2(-RANDOM_OFFSET, -RANDOM_OFFSET), {RANDOM_OFFSET, RANDOM_OFFSET});
    this->targetPosition = targetPoint + offset;
}

void Player::handleCollision(Mob* other) {
    if(this->killTarget && this->killTarget == other) {
        // the player wants to kill the mob and collided with him, execute the kill
        executeKill(this, other);
    }
}

Mob::~Mob() {
    if (this->body) {
        gameState.b2world->DestroyBody(this->body);
        this->body = nullptr;
    }
}