#include "deadfish.hpp"
#include <glm/gtx/vector_angle.hpp>
#include <iostream>

const float TURN_SPEED = .2f;
const float WALK_SPEED = 5.f;
const float CLOSE = 2.f;

std::ostream &operator<<(std::ostream &os, glm::vec2 &v)
{
    os << v.x << "," << v.y;
    return os;
}

void Mob::update()
{
    float dist = glm::distance(this->position, this->targetPosition);
    if (dist < CLOSE)
        return; // don't move if really close to target

    // update angle
    glm::vec2 toTarget = glm::normalize(this->targetPosition - this->position);
    float currentAngle = -glm::orientedAngle(toTarget, glm::vec2(1, 0)) + M_PI / 2;
    float diff = currentAngle - this->angle;

    if (diff < -M_PI)
        diff += 2 * M_PI;
    if (diff > M_PI)
        diff -= 2 * M_PI;

    bool skip = false;
    if (abs(diff) < TURN_SPEED)
    {
        this->angle += diff;
        skip = true;
    }
    if (!skip)
    {
        if (diff > 0)
        {
            this->angle += TURN_SPEED;
        }
        else
        {
            this->angle -= TURN_SPEED;
        }

        if (this->angle > M_PI)
            this->angle -= 2 * M_PI;
        if (this->angle < M_PI)
            this->angle += 2 * M_PI;
    }

    // update position
    glm::vec2 translation(1, 0);
    this->position += glm::rotate(translation, this->angle - (float)(M_PI / 2)) * WALK_SPEED;
}

void Player::update()
{
    Mob::update();
}