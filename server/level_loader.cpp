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

flatbuffers::Offset<FlatBuffGenerated::Level> serializeLevel(flatbuffers::FlatBufferBuilder &builder)
{
	//tilesets
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Tileset>> tilesetOffsets;
	for (auto &ts : gameState.level->tilesets) {
		auto path = builder.CreateString(ts->path);
		auto offset = FlatBuffGenerated::CreateTileset(builder, path, ts->firstgid);
		tilesetOffsets.push_back(offset);
	}
	auto tilesets = builder.CreateVector(tilesetOffsets);

	// visible
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Visible>> visibleOffsets;
	for (auto &v : gameState.level->visible) {
		FlatBuffGenerated::Vec2 pos(v->pos.x, v->pos.y);
		auto offset = FlatBuffGenerated::CreateVisible(builder, &pos, v->rotation, v->gid);
		visibleOffsets.push_back(offset);
	}
	auto visible = builder.CreateVector(visibleOffsets);

	// hiding spots
	std::vector<flatbuffers::Offset<FlatBuffGenerated::HidingSpot>> hspotOffsets;
	for (auto &hs : gameState.level->hidingspots)
	{
		FlatBuffGenerated::Vec2 pos(hs->body->GetPosition().x, hs->body->GetPosition().y);
		bool ellipse = false;
		float radius = 0;
		flatbuffers::Offset<flatbuffers::Vector<const FlatBuffGenerated::Vec2 *>> polyverts = 0;

		auto f = hs->body->GetFixtureList();
		if (auto circleShape = dynamic_cast<b2CircleShape*>(f->GetShape())) {
			ellipse = true;
			radius = circleShape->m_radius;
		} else {
			auto polyShape = dynamic_cast<b2PolygonShape*>(f->GetShape());
			std::vector<const FlatBuffGenerated::Vec2* > temppolyverts;
			for (auto vertex : polyShape->m_vertices) {
				const auto v = std::make_unique<FlatBuffGenerated::Vec2>(vertex.x, vertex.y);
				temppolyverts.push_back(std::move(v.get()));
			}
			polyverts = builder.CreateVector(temppolyverts);
		}
		auto off = FlatBuffGenerated::CreateHidingSpot(builder, &pos, ellipse, radius, polyverts);
		hspotOffsets.push_back(off);
	}
	auto hidingspots = builder.CreateVector(hspotOffsets);

	// final
	FlatBuffGenerated::Vec2 size(gameState.level->size.x, gameState.level->size.y);
	auto level = FlatBuffGenerated::CreateLevel(builder, visible, hidingspots, 0, 0, 0, tilesets, &size);
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

	//visible
	for (size_t i = 0; i < level->visible()->size(); i++)
	{
		auto visible = level->visible()->Get(i);
		auto vs = std::make_unique<Visible>(visible);
		gameState.level->visible.push_back(std::move(vs));
	}	

	// hiding spots
	for (size_t i = 0; i < level->hidingspots()->size(); i++)
	{
		auto hspot = level->hidingspots()->Get(i);
		auto hs = std::make_unique<HidingSpot>(hspot);
		gameState.level->hidingspots.push_back(std::move(hs));
	}

	// collisions
	for (size_t i = 0; i < level->collision()->size(); i++)
	{
		auto stone = level->collision()->Get(i);
		auto s = std::make_unique<Collision>(stone);
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
