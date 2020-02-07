#include <fstream>
#include <flatbuffers/flatbuffers.h>
#include "deadfish.hpp"
#include "level_loader.hpp"

void initStone(Stone* s, const DeadFish::Stone* dfstone) {
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

flatbuffers::Offset<DeadFish::Level> serializeLevel(flatbuffers::FlatBufferBuilder& builder) {
    // bushes
    std::vector<flatbuffers::Offset<DeadFish::Bush>> bushOffsets;
    for (auto& b : gameState.level->bushes) {
        DeadFish::Vec2 pos(b->position.x, b->position.y);
        auto off = DeadFish::CreateBush(builder, b->radius, &pos);
        bushOffsets.push_back(off);
    }
    auto bushes = builder.CreateVector(bushOffsets);

    // stones
    std::vector<flatbuffers::Offset<DeadFish::Stone>> stoneOffsets;
    for (auto& s : gameState.level->stones) {
        DeadFish::Vec2 pos(s->body->GetPosition().x, s->body->GetPosition().y);
        auto f = s->body->GetFixtureList();
        auto c = (b2CircleShape*) f->GetShape();
        auto off = DeadFish::CreateStone(builder, c->m_radius, &pos);
        stoneOffsets.push_back(off);
    }
    auto stones = builder.CreateVector(stoneOffsets);

    // the client doesn't need these so just leave them empty
    std::vector<flatbuffers::Offset<DeadFish::NavPoint>> navpoints;
    std::vector<const DeadFish::Vec2*> playerpoints;

    auto navOff = builder.CreateVector(navpoints);
    auto pointsOff = builder.CreateVector(playerpoints);

    DeadFish::Vec2 size(gameState.level->size.x, gameState.level->size.y);
    auto level = DeadFish::CreateLevel(builder, bushes, stones, navOff, pointsOff, &size);
    return level;
}

std::ostream& operator<<(std::ostream& os, std::vector<std::string>& v) {
    for (auto s : v) {
        os << s << ",";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, NavPoint& n) {
    os << n.isspawn << "\t" << n.position << "\t" << n.neighbors;
    return os;
}

void loadLevel(std::string& path) {
    std::ifstream in;
    in.open(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        std::cout << "failed to open level file " << path << "\n";
        exit(1);
    }
    auto size = in.tellg();
    auto memblock = new char [size];
    in.seekg (0, std::ios::beg);
    in.read (memblock, size);
    in.close();

    auto level = flatbuffers::GetRoot<DeadFish::Level>(memblock);

    // bushes
    for (int i = 0; i < level->bushes()->size(); i++) {
        auto bush = level->bushes()->Get(i);
        auto b = new Bush;
        b->position = glm::vec2(bush->pos()->x(), bush->pos()->y());
        b->radius = bush->radius();
        gameState.level->bushes.push_back(std::unique_ptr<Bush>(b));
    }

    // stones
    for (int i = 0; i < level->stones()->size(); i++) {
        auto stone = level->stones()->Get(i);
        auto s = new Stone;
        initStone(s, stone);
        gameState.level->stones.push_back(std::unique_ptr<Stone>(s));
    }

    // navpoints
    for (int i = 0; i < level->navpoints()->size(); i++) {
        auto navpoint = level->navpoints()->Get(i);
        auto n = new NavPoint;
        n->isspawn = navpoint->isspawn();
        n->isplayerspawn = navpoint->isplayerspawn();
        n->position = glm::vec2(navpoint->position()->x(), navpoint->position()->y());
        for (int j = 0; j < navpoint->neighbors()->size(); j++) {
            n->neighbors.push_back(navpoint->neighbors()->Get(j)->c_str());
        }
        gameState.level->navpoints[navpoint->name()->c_str()] = std::unique_ptr<NavPoint>(n);
    }
} 