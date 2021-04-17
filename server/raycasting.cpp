#include "raycasting.hpp"

bool playerSeeCollideable(Player &p, Collideable &c)
{
    struct Data {
        float minfraction = 1.f;
        b2Fixture *closest = nullptr;
    };
    FOVCallback<Data> fovCb;
    fovCb.f = [player = &p, target = &c](b2Fixture *fixture, UNUSED const b2Vec2 &point, UNUSED const b2Vec2 &normal,
        float32 fraction, Data& data) {
        auto collideable = (Collideable *)fixture->GetBody()->GetUserData();
		if (collideable && collideable != target && !collideable->obstructsSight(player))
			return 1.f;
		if (fraction < data.minfraction)
		{
			data.minfraction = fraction;
			data.closest = fixture;
		}
		return fraction;
    };
    auto ppos = p.deathTimeout > 0 ? g2b(p.targetPosition) : p.body->GetPosition();
	auto cpos = c.body->GetPosition();
	gameState.b2world->RayCast(&fovCb, ppos, cpos);
    return fovCb.data.closest && fovCb.data.closest->GetBody() == c.body;
}

bool mobSeePoint(Mob &m, const b2Vec2 &point, bool ignoreMobs, bool ignorePlayerwalls, bool ignoreHidingSpots)
{
	if (b2Distance(m.body->GetPosition(), point) == 0.0f)
		return true;
    struct Data {
        float minfraction = 1.f;
    };
	FOVCallback<Data> fovCb;
    fovCb.f = [ignoreMobs, ignorePlayerwalls, ignoreHidingSpots]
		(b2Fixture *fixture, UNUSED const b2Vec2 &point, UNUSED const b2Vec2 &normal,
        float32 fraction, Data& data){
        auto collideable = (Collideable *)fixture->GetBody()->GetUserData();
		// on return 1.f the currently reported fixture will be ignored and the raycast will continue
		if (ignoreMobs && dynamic_cast<Mob*>(collideable))
			return 1.f;
        if (ignorePlayerwalls && dynamic_cast<PlayerWall*>(collideable))
			return 1.f;
		if (ignoreHidingSpots && dynamic_cast<HidingSpot*>(collideable))
			return 1.f;
		if (fraction < data.minfraction)
		{
			data.minfraction = fraction;
		}
		return fraction;
    };
	gameState.b2world->RayCast(&fovCb, m.body->GetPosition(), point);
	return fovCb.data.minfraction == 1.f;
}
