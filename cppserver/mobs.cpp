#include "deadfish.hpp"
#include <glm/gtx/vector_angle.hpp>
#include <iostream>

const float TURN_SPEED = 4.f;
const float WALK_SPEED = 1.f;
const float CLOSE = 0.05f;
const float ANGULAR_CLOSE = 0.1f;

std::ostream &operator<<(std::ostream &os, glm::vec2 &v)
{
    os << v.x << "," << v.y;
    return os;
}

void Mob::update()
{
    float dist = glm::distance(b2g(this->body->GetPosition()), this->targetPosition);
    auto pos = b2g(this->body->GetPosition());
    if (dist < CLOSE) {
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
    std::cout << "diff\t" << diff << "\n";
    auto turnSpeed = TURN_SPEED;
    if (abs(diff) < ANGULAR_CLOSE*2)
    {
        turnSpeed /= 2.;
    }
    if (abs(diff) < ANGULAR_CLOSE) {
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
    auto translation = glm::rotate(glm::vec2(1, 0), this->body->GetAngle() - (float)(M_PI / 2)) * WALK_SPEED;
    this->body->SetLinearVelocity(b2Vec2(translation.x, translation.y));

    // update box
    // this->body->SetTransform(b2Vec2(this->position.x, this->position.y), this->angle);
    // std::cout << "physics pos \t" << this->body->GetPosition().x << "\t" << this->body->GetPosition().y << "\n";
}

void Player::update()
{
    Mob::update();
}