#include "skills.hpp"
#include "../common/geometry.hpp"

uint16_t lastInkID = 0;

void launchInkParticle(Player& p, b2Vec2 direction) {
	// create the ink body
	b2BodyDef myBodyDef = {};
	myBodyDef.type = b2_dynamicBody;      //this will be a dynamic body
	myBodyDef.position = p.body->GetPosition(); //set the starting position
	auto inkBody = gameState.b2world->CreateBody(&myBodyDef);
	b2CircleShape circleShape;
	circleShape.m_radius = 0.6f;

	auto inkPart = std::make_unique<InkParticle>(inkBody, p);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circleShape;
	fixtureDef.density = 1;
	fixtureDef.friction = 0;
	fixtureDef.isSensor = true;
	fixtureDef.filter.categoryBits = 1; // collide with all mobs
	fixtureDef.filter.maskBits = 0xFFFF;
	inkBody->CreateFixture(&fixtureDef);
	inkBody->SetUserData(inkPart.get());

	inkBody->SetLinearDamping(1);
	inkBody->ApplyLinearImpulse(direction, inkBody->GetWorldCenter(), true);

	inkPart->inkID = lastInkID++;
	gameState.inkParticles.push_back(std::move(inkPart));
}

const float INK_INIT_SPEED_BASE = 0.5;
const float INK_INIT_SPEED_VARIABLE = 1.5;
const int INK_COUNT = 5;
const int INK_LIFETIME_FRAMES = 80; // 4s

void executeSkillInkbomb(Player& p, UNUSED Skills skill, UNUSED b2Vec2 mousePos) {
	std::cout << "ink bomb\n";
	for (int i = 0; i < INK_COUNT; i++) {
		b2Vec2 direction(INK_INIT_SPEED_BASE + (rand() % ((int)(INK_INIT_SPEED_VARIABLE * 10)))/10.f, 0);
		int dirDeg = rand() % 360;
		direction = rotateVector(direction, dirDeg * (M_PI / 180.f));
		launchInkParticle(p, direction);
	}
}

InkParticle::InkParticle(b2Body* b, Player& o) 
	: owner(o) {
	this->body = b;
	this->lifetimeFrames = INK_LIFETIME_FRAMES;
}

InkParticle::~InkParticle() {
	for (b2ContactEdge* edge = body->GetContactList(); edge; edge = edge->next) {
		auto collideableA = (Collideable *) edge->contact->GetFixtureA()->GetBody()->GetUserData();
		auto collideableB = (Collideable *) edge->contact->GetFixtureB()->GetBody()->GetUserData();

		auto mobA = dynamic_cast<Mob *>(collideableA);
		auto mobB = dynamic_cast<Mob *>(collideableB);

		if (mobA) {
			mobA->bombAffected = false;
			mobA->speed = WALK_SPEED;
		}
		
		if (mobB) {
			mobB->bombAffected = false;
			mobB->speed = WALK_SPEED;
		}
	}
	gameState.b2world->DestroyBody(this->body);
}

void InkParticle::update() {
	this->lifetimeFrames--;
	if (this->lifetimeFrames == 0) {
		this->toBeDeleted = true;
		std::cout << "ink particle toBeDeleted\n";
	}
}

void InkParticle::handleCollision(Collideable& other) {
	try {
		auto& p = dynamic_cast<Player&>(other);
		if (p.playerID == this->owner.playerID)
			return; // bombs don't affects the players that threw them
	} catch (...) {}
	try {
		auto& m = dynamic_cast<Mob&>(other);
		m.bombAffected = true;
		m.speed *= INK_BOMB_SPEED_MODIFIER;
	} catch (...) {}
}

void InkParticle::endCollision(Collideable& other) {
	try {
		auto& m = dynamic_cast<Mob&>(other);
		m.bombAffected = false;
		m.speed = WALK_SPEED;
	} catch (...) {}
}

bool InkParticle::obstructsSight(UNUSED Player* p) {
	return false;
}

void executeSkillMobManipulator(UNUSED Player& p, UNUSED Skills skill, UNUSED b2Vec2 mousePos) {
	std::cout << "mob manipulator " << (uint16_t) skill << "\n";
}

void executeSkillBlink(UNUSED Player& p, UNUSED Skills skill, UNUSED b2Vec2 mousePos) {
	std::cout << "skill blink\n";
}
