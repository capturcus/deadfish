#pragma once

#include "deadfish.hpp"

bool mobSeePoint(Mob &m, const b2Vec2 &point, bool ignoreMobs = false,
	bool ignorePlayerwalls = false, bool ignoreHidingSpots = false);
bool playerSeeCollideable(Player &p, Collideable &c);

typedef std::function<float(b2Fixture*, const b2Vec2 &, const b2Vec2 &, float32)> RaycastingCallback;

class FOVCallback final : public b2RayCastCallback
{
private:
	RaycastingCallback _f;
public:
	FOVCallback(RaycastingCallback&& f) : _f(std::move(f)) {}

	float32 ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float32 fraction) override {
	return _f(fixture, point, normal, fraction);
	}
};
