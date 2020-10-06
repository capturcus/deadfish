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

	// objects
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Object>> objectOffsets;
	for (auto &v : gameState.level->objects) {
		FlatBuffGenerated::Vec2 pos(v->pos.x, v->pos.y);
		auto offset = FlatBuffGenerated::CreateObject(builder, &pos, v->rotation, v->gid);
		objectOffsets.push_back(offset);
	}
	auto objects = builder.CreateVector(objectOffsets);

	// decoration
	std::vector<flatbuffers::Offset<FlatBuffGenerated::Decoration>> decorationOffsets;
	for (auto &v : gameState.level->decoration) {
		FlatBuffGenerated::Vec2 pos(v->pos.x, v->pos.y);
		auto offset = FlatBuffGenerated::CreateDecoration(builder, &pos, v->rotation, v->gid);
		decorationOffsets.push_back(offset);
	}
	auto decoration = builder.CreateVector(decorationOffsets);

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
	auto level = FlatBuffGenerated::CreateLevel(builder, objects, decoration, hidingspots, 0, 0, 0, tilesets, &size);
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

	// tilesets
	for (auto tileset : *level->tilesets()) {
		auto ts = std::make_unique<Tileset>(tileset);
		gameState.level->tilesets.push_back(std::move(ts));
	}

	// objects
	for (auto object : *level->objects())
	{
		auto o = std::make_unique<Object>(object);
		gameState.level->objects.push_back(std::move(o));
	}

	// decoration
	for (auto decoration : *level->decoration())
	{
		auto d = std::make_unique<Decoration>(decoration);
		gameState.level->decoration.push_back(std::move(d));
	}

	// hiding spots
	for (auto hspot : *level->hidingspots())
	{
		auto hs = std::make_unique<HidingSpot>(hspot);
		gameState.level->hidingspots.push_back(std::move(hs));
	}

	// collisions
	for (auto collision : *level->collision())
	{
		auto s = std::make_unique<Collision>(collision);
		gameState.level->collisions.push_back(std::move(s));
	}

	// playerwalls
	for (auto playerwall : *level->playerwalls())
	{
		initPlayerwall(playerwall);
	}

	// navpoints
	for (auto navpoint : *level->navpoints())
	{
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
