#include <fstream>
#include <flatbuffers/flatbuffers.h>
#include "deadfish.hpp"
#include "level_loader.hpp"

void initPlayerwall(const DeadFish::PlayerWall *pw)
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

void initStone(Stone *s, const DeadFish::Stone *dfstone)
{
	b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(dfstone->pos()->x(), dfstone->pos()->y());
	myBodyDef.angle = 0;
	s->body = gameState.b2world->CreateBody(&myBodyDef);
	b2CircleShape circleShape;
	circleShape.m_radius = dfstone->radius();

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circleShape;
	fixtureDef.density = 1;
	s->body->CreateFixture(&fixtureDef);
	s->body->SetUserData(s);
}

flatbuffers::Offset<DeadFish::Level> serializeLevel(flatbuffers::FlatBufferBuilder &builder)
{
	// bushes
	std::vector<flatbuffers::Offset<DeadFish::Bush>> bushOffsets;
	for (auto &b : gameState.level->bushes)
	{
		DeadFish::Vec2 pos(b->position.x, b->position.y);
		auto off = DeadFish::CreateBush(builder, b->radius, &pos);
		bushOffsets.push_back(off);
	}
	auto bushes = builder.CreateVector(bushOffsets);

	// stones
	std::vector<flatbuffers::Offset<DeadFish::Stone>> stoneOffsets;
	for (auto &s : gameState.level->stones)
	{
		DeadFish::Vec2 pos(s->body->GetPosition().x, s->body->GetPosition().y);
		auto f = s->body->GetFixtureList();
		auto c = (b2CircleShape *)f->GetShape();
		auto off = DeadFish::CreateStone(builder, c->m_radius, &pos);
		stoneOffsets.push_back(off);
	}
	auto stones = builder.CreateVector(stoneOffsets);

	DeadFish::Vec2 size(gameState.level->size.x, gameState.level->size.y);
	auto level = DeadFish::CreateLevel(builder, bushes, stones, 0, 0, &size);
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

	auto level = flatbuffers::GetRoot<DeadFish::Level>(memblock.data());

	// bushes
	for (size_t i = 0; i < level->bushes()->size(); i++)
	{
		auto bush = level->bushes()->Get(i);
		auto b = std::make_unique<Bush>();
		b->position = glm::vec2(bush->pos()->x(), bush->pos()->y());
		b->radius = bush->radius();
		gameState.level->bushes.push_back(std::move(b));
	}

	// stones
	for (size_t i = 0; i < level->stones()->size(); i++)
	{
		auto stone = level->stones()->Get(i);
		auto s = std::make_unique<Stone>();
		initStone(s.get(), stone);
		gameState.level->stones.push_back(std::move(s));
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