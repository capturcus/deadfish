#include "skills.hpp"

uint16_t lastInkID = 0;

void executeSkillInkbomb(Player& p, Skills skill, b2Vec2 mousePos) {
	std::cout << "ink bomb\n";
	// create the ink body
	b2BodyDef myBodyDef = {};
	myBodyDef.type = b2_dynamicBody;      //this will be a dynamic body
	myBodyDef.position = p.body->GetPosition(); //set the starting position
	auto inkBody = gameState.b2world->CreateBody(&myBodyDef);
	b2CircleShape circleShape;
	circleShape.m_radius = 0.6f;

	auto inkPart = std::make_unique<InkParticle>(inkBody);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circleShape;
	fixtureDef.density = 1;
	fixtureDef.friction = 0;
	fixtureDef.isSensor = true;
	fixtureDef.filter.categoryBits = 1; // collide with all mobs
	fixtureDef.filter.maskBits = 0xFFFF;
	inkBody->CreateFixture(&fixtureDef);
	inkBody->SetUserData(inkPart.get());

	inkPart->inkID = lastInkID++;
	gameState.inkParticles.push_back(std::move(inkPart));
}

InkParticle::InkParticle(b2Body* b) {
	this->body = b;
}

InkParticle::~InkParticle() {
	gameState.b2world->DestroyBody(this->body);
}

void InkParticle::handleCollision(Collideable& other) {
	std::cout << "ink particle collision\n";
	try {
		auto& player = dynamic_cast<Player &>(other);
		player.targetPosition = b2g(player.body->GetPosition());
	} catch (...) {
		// nothing
	}
}

bool InkParticle::obstructsSight(Player* p) {
	return false;
}

void executeSkillMobManipulator(Player& p, Skills skill, b2Vec2 mousePos) {
	std::cout << "mob manipulator " << (uint16_t) skill << "\n";
}

void executeSkillBlink(Player& p, Skills skill, b2Vec2 mousePos) {
	std::cout << "skill blink\n";
}
