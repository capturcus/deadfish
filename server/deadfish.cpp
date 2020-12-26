#include "deadfish.hpp"
#include "../common/constants.hpp"

// HidingSpot

HidingSpot::HidingSpot(const FlatBuffGenerated::HidingSpot* fb_Hs) {
	this->name = fb_Hs->name()->str();
	b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(fb_Hs->pos()->x(), fb_Hs->pos()->y());
	myBodyDef.angle = fb_Hs->rotation() * TO_RADIANS;
	body = gameState.b2world->CreateBody(&myBodyDef);

	b2FixtureDef fixtureDef;
	fixtureDef.isSensor = true; // makes hiding spot detect collision but allow movement
	if (!fb_Hs->polyverts()) {
		if(fb_Hs->isCircle()){
			b2CircleShape circleShape;
			circleShape.m_radius = fb_Hs->size()->x() / 2.f;
			fixtureDef.shape = &circleShape;
		} else {
			b2PolygonShape boxShape;
			boxShape.SetAsBox(fb_Hs->size()->x()/2.f, fb_Hs->size()->y()/2.f);
			fixtureDef.shape = &boxShape;
		}
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
	}
	catch (...) {
		std::cout << "non-player collision with bush detected!" << std::endl;
	}
}

void HidingSpot::endCollision(Collideable& other) {
	this->playersInside.erase(dynamic_cast<Player*>(&other));
}

bool HidingSpot::obstructsSight(Player* p) {
	// obstructs sight if player is not inside
	return playersInside.find(p) == playersInside.end();
}

// CollisionMask

CollisionMask::CollisionMask(const FlatBuffGenerated::CollisionMask* fb_Col) {
	b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(fb_Col->pos()->x(), fb_Col->pos()->y());
	myBodyDef.angle = fb_Col->rotation() * TO_RADIANS;
	body = gameState.b2world->CreateBody(&myBodyDef);

	b2FixtureDef fixtureDef;
	if (!fb_Col->polyverts()) {
		if(fb_Col->isCircle()){
			b2CircleShape circleShape;
			circleShape.m_radius = fb_Col->size()->x() / 2.f;
			fixtureDef.shape = &circleShape;
		} else {
			b2PolygonShape boxShape;
			boxShape.SetAsBox(fb_Col->size()->x()/2.f, fb_Col->size()->y()/2.f);
			fixtureDef.shape = &boxShape;
		}
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

flatbuffers::Offset<FlatBuffGenerated::CollisionMask> CollisionMask::serialize(
	flatbuffers::FlatBufferBuilder &builder
) {
	FlatBuffGenerated::Vec2 pos{body->GetPosition().x, body->GetPosition().y};
	float rotation = body->GetAngle() * TO_DEGREES;

	b2Fixture* fixture = body->GetFixtureList();
	assert(fixture != nullptr);
	b2Shape* shape = fixture->GetShape();
	auto type = shape->GetType();

	if (type == b2Shape::e_polygon) {
		b2PolygonShape* polyShape = dynamic_cast<b2PolygonShape*>(shape);
		int32_t vertCount = polyShape->GetVertexCount();

		std::vector<FlatBuffGenerated::Vec2> polyverts_v;
		polyverts_v.reserve((size_t)vertCount);

		for (int32_t i = 0; i < vertCount; i++) {
			auto v = polyShape->GetVertex(i);
			polyverts_v.emplace_back(v.x, v.y);
		}
		auto polyverts = builder.CreateVectorOfStructs(polyverts_v);

		return FlatBuffGenerated::CreateCollisionMask(
			builder, &pos, nullptr, rotation, false, polyverts);

	} else if (type == b2Shape::e_circle) {
		b2CircleShape* circleShape = dynamic_cast<b2CircleShape*>(shape);
		FlatBuffGenerated::Vec2 size{circleShape->m_radius * 2.f, 0.f};

		return FlatBuffGenerated::CreateCollisionMask(
			builder, &pos, &size, rotation, true, 0);

	} else {
		assert(false); // unsupported shape type
		return 0;
	}
}
