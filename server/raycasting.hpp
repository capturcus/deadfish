#pragma once

#include "deadfish.hpp"

bool mobSeePoint(Mob &m, const b2Vec2 &point, bool ignoreMobs = false,
	bool ignorePlayerwalls = false, bool ignoreHidingSpots = false);
bool playerSeeCollideable(Player &p, Collideable &c);

template<typename S>
struct FOVCallback
	: public b2RayCastCallback
{
	float32 ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float32 fraction)
	{
        return f(fixture, point, normal, fraction, data);
	}
    std::function<float(b2Fixture*, const b2Vec2 &, const b2Vec2 &, float32, S& data)> f;
    S data;
};
