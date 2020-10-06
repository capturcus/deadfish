#include "deadfish.hpp"
#include "../common/constants.hpp"

// HidingSpot

HidingSpot::HidingSpot(const FlatBuffGenerated::HidingSpot* fb_Hs) {
    b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(fb_Hs->pos()->x(), fb_Hs->pos()->y());
	body = gameState.b2world->CreateBody(&myBodyDef);

	b2FixtureDef fixtureDef;
    fixtureDef.isSensor = true; // makes hiding spot detect collision but allow movement
    if (fb_Hs->ellipse()) {
        b2CircleShape circleShape;
        circleShape.m_radius = fb_Hs->radius();
        fixtureDef.shape = &circleShape;
    } else {
        b2PolygonShape polyShape;
        std::vector<b2Vec2> vertices;
        auto fb_poly = fb_Hs->polyverts();
        for (auto vert = fb_poly->begin(); vert != fb_poly->end(); ++vert) {
            vertices.push_back(b2Vec2(vert->x(), vert->y()));
        }
        polyShape.Set(vertices.data(), vertices.size());
        fixtureDef.shape = &polyShape;
    }
	fixtureDef.filter.categoryBits = 0x0010;    // "i be bush"
    fixtureDef.filter.maskBits = 0x0002;        // "i collide with player"
	body->CreateFixture(&fixtureDef);
	body->SetUserData(this);
}

void HidingSpot::handleCollision(Collideable& other) {
    try {
        auto &player = dynamic_cast<Player &>(other);
        playersInside.insert(&player);
        std::cout << "player entered the hiding spot" << std::endl;
    }
    catch (...) {
        std::cout << "non-player collision with bush detected!" << std::endl;
    }
}

bool HidingSpot::obstructsSight(Player* p) {
    // obstructs sight if player is not inside
    return playersInside.find(p) == playersInside.end();
}

// Collision

Collision::Collision(const FlatBuffGenerated::Collision* fb_Col) {
    b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(fb_Col->pos()->x(), fb_Col->pos()->y());
    myBodyDef.angle = fb_Col->rotation() * TO_RADIANS;
	body = gameState.b2world->CreateBody(&myBodyDef);

	b2FixtureDef fixtureDef;
    if (fb_Col->ellipse()) {
        b2CircleShape circleShape;
        circleShape.m_radius = fb_Col->radius();
        fixtureDef.shape = &circleShape;
    } else {
        b2PolygonShape polyShape;
        std::vector<b2Vec2> vertices;
        for (auto vert : *fb_Col->polyverts()) {
            vertices.push_back(b2Vec2(vert->x(), vert->y()));
        }
        polyShape.Set(vertices.data(), vertices.size());
        fixtureDef.shape = &polyShape;
    }
    fixtureDef.density = 1;
	body->CreateFixture(&fixtureDef);
	body->SetUserData(this);
}
