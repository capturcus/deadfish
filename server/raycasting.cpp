#include "raycasting.hpp"

bool playerSeeCollideable(Player &p, Collideable &c)
{
	float min_fraction = 1.f;
	b2Fixture *closest = nullptr;
	auto fovCb = FOVCallback([player = &p, target = &c, &min_fraction, &closest]
		(b2Fixture *fixture, UNUSED const b2Vec2 &point, UNUSED const b2Vec2 &normal,
		float32 fraction) {
		auto collideable = (Collideable *)fixture->GetBody()->GetUserData();
		if (collideable && collideable != target && !collideable->obstructsSight(player))
			return 1.f;
		if (fraction < min_fraction)
		{
			min_fraction = fraction;
			closest = fixture;
		}
		return fraction;
	});
	auto ppos = p.deathTimeout > 0 ? g2b(p.targetPosition) : p.body->GetPosition();
	auto cpos = c.body->GetPosition();
	gameState.b2world->RayCast(&fovCb, ppos, cpos);
	return closest && closest->GetBody() == c.body;
}

bool mobSeePoint(Mob &m, const b2Vec2 &point, bool ignoreMobs, bool ignorePlayerwalls, bool ignoreHidingSpots)
{
	if (b2Distance(m.body->GetPosition(), point) == 0.0f)
		return true;
	float min_fraction = 1.f;
	auto fovCb = FOVCallback([ignoreMobs, ignorePlayerwalls, ignoreHidingSpots, &min_fraction]
		(b2Fixture *fixture, UNUSED const b2Vec2 &point, UNUSED const b2Vec2 &normal,
	float32 fraction){
	auto collideable = (Collideable *)fixture->GetBody()->GetUserData();
		// on return 1.f the currently reported fixture will be ignored and the raycast will continue
		if (ignoreMobs && dynamic_cast<Mob*>(collideable))
			return 1.f;
	if (ignorePlayerwalls && dynamic_cast<PlayerWall*>(collideable))
			return 1.f;
		if (ignoreHidingSpots && dynamic_cast<HidingSpot*>(collideable))
			return 1.f;
		min_fraction = std::min(min_fraction, fraction);
		return fraction;
	});
	gameState.b2world->RayCast(&fovCb, m.body->GetPosition(), point);
	return min_fraction == 1.f;
}
