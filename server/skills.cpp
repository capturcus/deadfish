#include "skills.hpp"
#include "game_thread.hpp"
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

	inkBody->SetLinearDamping(1);
	inkBody->ApplyLinearImpulse(direction, inkBody->GetWorldCenter(), true);

	inkPart->inkID = lastInkID++;
	gameState.inkParticles.push_back(std::move(inkPart));
}

const float INK_INIT_SPEED_BASE = 2;
const float INK_INIT_SPEED_VARIABLE = 1.5;
const int INK_COUNT = 5;
const int INK_LIFETIME_FRAMES = 80; // 4s

bool executeSkillInkbomb(Player& p, UNUSED Skills skill, b2Vec2 mousePos) {
	std::cout << "ink bomb\n";
	b2Vec2 mouseVector = mousePos - p.body->GetPosition();
	float mouseAngleDeg = angleFromVector(mouseVector) * TO_DEGREES;
	for (int i = 0; i < INK_COUNT; i++) {
		float mouseAngleRandomized = mouseAngleDeg + rand() % 90 - 45;
		b2Vec2 direction(INK_INIT_SPEED_BASE + (rand() % ((int)(INK_INIT_SPEED_VARIABLE * 10)))/10.f, 0);
		direction = rotateVector(direction, mouseAngleRandomized * TO_RADIANS);
		launchInkParticle(p, direction);
	}
	return true;
}

InkParticle::InkParticle(b2Body* b)
{
	this->body = b;
	this->lifetimeFrames = INK_LIFETIME_FRAMES;
}

InkParticle::~InkParticle() {
	for (b2ContactEdge* edge = body->GetContactList(); edge; edge = edge->next) {
		if (edge->contact->IsTouching()) {
			auto collideableA = (Collideable *) edge->contact->GetFixtureA()->GetBody()->GetUserData();
			auto collideableB = (Collideable *) edge->contact->GetFixtureB()->GetBody()->GetUserData();

			auto mobA = dynamic_cast<Mob *>(collideableA);
			auto mobB = dynamic_cast<Mob *>(collideableB);

			if (mobA && mobA->bombsAffecting > 0)
				mobA->bombsAffecting--;

			if (mobB && mobB->bombsAffecting > 0)
				mobB->bombsAffecting--;
		}
	}
	gameState.b2world->DestroyBody(this->body);
}

void InkParticle::update() {
	this->lifetimeFrames--;
	if (this->lifetimeFrames == 0)
		this->toBeDeleted = true;
}

void InkParticle::handleCollision(Collideable& other) {
	try {
		auto& m = dynamic_cast<Mob&>(other);
		m.bombsAffecting++;
	} catch (...) {}
}

void InkParticle::endCollision(Collideable& other) {
	try {
		auto& m = dynamic_cast<Mob&>(other);
		if (m.bombsAffecting > 0)
			m.bombsAffecting--;
	} catch (...) {}
}

bool InkParticle::obstructsSight(UNUSED Player* p) {
	return true;
}

const uint16_t MANIPULATOR_FRAMES = 60;

bool executeSkillMobManipulator(UNUSED Player& p, UNUSED Skills skill, UNUSED b2Vec2 mousePos) {
	std::cout << "mob manipulator " << (uint16_t) skill << "\n";
	MobManipulator manipulator;
	manipulator.pos = p.body->GetPosition();
	manipulator.type = skill == Skills::DISPERSOR ? FlatBuffGenerated::MobManipulatorType_Dispersor
		: FlatBuffGenerated::MobManipulatorType_Attractor;
	manipulator.framesLeft = MANIPULATOR_FRAMES;
	gameState.mobManipulators.push_back(manipulator);
	return true;
}

bool executeSkillBlink(Player& p, UNUSED Skills skill, b2Vec2 mousePos) {
	if (b2Distance(mousePos, p.body->GetPosition()) > BLINK_RANGE)
		return false; // ignore blinks out of range
	gameState.b2world->DestroyBody(p.body);
	physicsInitMob(&p, b2g(mousePos), 0, 0.3f, 3);
	return true;
}
