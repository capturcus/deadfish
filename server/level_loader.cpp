#include <fstream>
#include <flatbuffers/flatbuffers.h>
#include "deadfish.hpp"
#include "level_loader.hpp"

void initPlayerwall(const FlatBuffGenerated::PlayerWall *pw)
{
	std::cout << "playerwall " << pw->position()->x() << "," << pw->position()->y() << "; " << pw->size()->x() << "," << pw->size()->y() << "\n";
	b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(pw->position()->x(), pw->position()->y());
	b2Body *staticBody = gameState.b2world->CreateBody(&myBodyDef); //add body to world
	b2PolygonShape boxShape;
	boxShape.SetAsBox(pw->size()->x(), pw->size()->y());
	b2FixtureDef boxFixtureDef;
	boxFixtureDef.shape = &boxShape;
	boxFixtureDef.density = 1;
	boxFixtureDef.filter.maskBits = ~1;
	boxFixtureDef.filter.categoryBits = 1 << 1;
	staticBody->CreateFixture(&boxFixtureDef); //add fixture to body
	gameState.level->playerwalls.emplace_back(std::make_unique<PlayerWall>());
	staticBody->SetUserData(gameState.level->playerwalls.back().get());
	gameState.level->playerwalls.back()->body = staticBody;
}

void initStone(Collision *s, const FlatBuffGenerated::Collision *dfstone)
{
	b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(dfstone->pos()->x(), dfstone->pos()->y());
	myBodyDef.angle = -dfstone->rotation();
	s->body = gameState.b2world->CreateBody(&myBodyDef);
	b2CircleShape circleShape;
	circleShape.m_radius = dfstone->radius();

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circleShape;
	fixtureDef.density = 1;
	s->body->CreateFixture(&fixtureDef);
	s->body->SetUserData(s);
}

void initHidingSpot(HidingSpot *b, const FlatBuffGenerated::HidingSpot *dfhspot)
{
	b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(dfhspot->pos()->x(), dfhspot->pos()->y());
	myBodyDef.angle = -dfhspot->rotation();
	b->body = gameState.b2world->CreateBody(&myBodyDef);
	b2CircleShape circleShape;
	circleShape.m_radius = dfhspot->radius();

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circleShape;
	fixtureDef.density = 1;
	fixtureDef.filter.categoryBits = 0x0002;
	b->body->CreateFixture(&fixtureDef);
	b->body->SetUserData(b);
}

flatbuffers::Offset<FlatBuffGenerated::Level> serializeLevel(flatbuffers::FlatBufferBuilder &builder)
{
	// hiding spots
	std::vector<flatbuffers::Offset<FlatBuffGenerated::HidingSpot>> hspotOffsets;
	for (auto &b : gameState.level->hidingspots)
	{
		FlatBuffGenerated::Vec2 pos(b->body->GetPosition().x, b->body->GetPosition().y);
		auto f = b->body->GetFixtureList();
		auto c = (b2CircleShape *)f->GetShape();
		auto off = FlatBuffGenerated::CreateHidingSpot(builder, c->m_radius, b->body->GetAngle(), &pos);
		hspotOffsets.push_back(off);
	}
	auto hidingspots = builder.CreateVector(hspotOffsets);

	// collisions
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Collision>> stoneOffsets;
	for (auto &s : gameState.level->collisions)
	{
		FlatBuffGenerated::Vec2 pos(s->body->GetPosition().x, s->body->GetPosition().y);
		auto f = s->body->GetFixtureList();
		auto c = (b2CircleShape *)f->GetShape();
		auto off = FlatBuffGenerated::CreateStone(builder, c->m_radius, s->body->GetAngle(), &pos);
		stoneOffsets.push_back(off);
	}
	auto collisions = builder.CreateVector(stoneOffsets);

	FlatBuffGenerated::Vec2 size(gameState.level->size.x, gameState.level->size.y);
	auto level = FlatBuffGenerated::CreateLevel(builder, hidingspots, collisions, 0, 0, &size);
	return level;
}

std::ostream &operator<<(std::ostream &os, std::vector<std::string> &v)
{
	for (auto s : v)
	{
		os << s << ",";
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, NavPoint &n)
{
	os << n.isspawn << "\t" << n.position << "\t" << n.neighbors;
	return os;
}

void loadLevel(std::string &path)
{
	std::ifstream in;
	in.open(path, std::ios::in | std::ios::binary | std::ios::ate);
	if (!in.is_open())
	{
		std::cout << "failed to open level file " << path << "\n";
		exit(1);
	}
	auto size = in.tellg();
	std::vector<char> memblock(size);
	in.seekg(0, std::ios::beg);
	in.read(memblock.data(), memblock.size());
	in.close();

	auto level = flatbuffers::GetRoot<FlatBuffGenerated::Level>(memblock.data());

	// hiding spots
	for (size_t i = 0; i < level->hidingspots()->size(); i++)
	{
		auto hspot = level->hidingspots()->Get(i);
		auto hs = std::make_unique<HidingSpot>();
		initHidingSpot(hs.get(), hspot);
		gameState.level->hidingspots.push_back(std::move(hs));
	}

	// collisions
	for (size_t i = 0; i < level->collisions()->size(); i++)
	{
		auto stone = level->collisions()->Get(i);
		auto s = std::make_unique<Collision>();
		initStone(s.get(), stone);
		gameState.level->collisions.push_back(std::move(s));
	}

	// playerwalls
	for (size_t i = 0; i < level->playerwalls()->size(); i++)
	{
		auto playerwall = level->playerwalls()->Get(i);
		initPlayerwall(playerwall);
	}

	// navpoints
	for (size_t i = 0; i < level->navpoints()->size(); i++)
	{
		auto navpoint = level->navpoints()->Get(i);
		auto n = std::make_unique<NavPoint>();
		n->isspawn = navpoint->isspawn();
		n->isplayerspawn = navpoint->isplayerspawn();
		n->position = glm::vec2(navpoint->position()->x(), navpoint->position()->y());
		n->radius = navpoint->radius();
		for (size_t j = 0; j < navpoint->neighbors()->size(); j++)
		{
			n->neighbors.push_back(navpoint->neighbors()->Get(j)->c_str());
		}
		gameState.level->navpoints[navpoint->name()->c_str()] = std::move(n);
	}
}
