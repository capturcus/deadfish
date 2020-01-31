#include <iostream>
#include <unistd.h>
#include "deadfish.hpp"
#include "game_thread.hpp"
#include "level_loader.hpp"

const int FRAME_TIME = 50; // 20 fps
const int CIVILIAN_TIME = 40;
const int MAX_CIVILIANS = 6;

bool operator==(websocketpp::connection_hdl &a, websocketpp::connection_hdl &b)
{
    return a.lock().get() == b.lock().get();
}

uint16_t newID()
{
    while (true)
    {
        uint16_t ret = rand() % UINT16_MAX;

        for (auto &p : gameState.players)
        {
            if (p->id == ret)
                continue;
        }

        for (auto &n : gameState.civilians)
        {
            if (n->id == ret)
                continue;
        }

        return ret;
    }
}

Player *getPlayerByConnHdl(websocketpp::connection_hdl &hdl)
{
    auto player = gameState.players.begin();
    while (player != gameState.players.end())
    {
        if ((*player)->conn_hdl == hdl)
        {
            return player->get();
        }
        player++;
    }
    std::cout << "getPlayerByConnHdl PLAYER NOT FOUND\n";
    return nullptr;
}

void gameOnMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    const auto payload = msg->get_payload();
    const auto clientMessage = flatbuffers::GetRoot<DeadFish::ClientMessage>(payload.c_str());
    switch (clientMessage->event_type())
    {
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandMove:
    {
        const auto event = clientMessage->event_as_CommandMove();
        auto p = getPlayerByConnHdl(hdl);
        p->targetPosition = glm::vec2(event->target()->x(), event->target()->y());
    }
    break;

    default:
        std::cout << "gameOnMessage: some other message type received\n";
        break;
    }
}

struct FOVCallback
    : public b2RayCastCallback
{
    float32 ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float32 fraction)
    {
        if (fraction < minfraction)
        {
            minfraction = fraction;
            closest = fixture;
        }
        return fraction;
    }
    float minfraction = 1.f;
    b2Fixture *closest = nullptr;
};

bool playerSeeMob(Player *const p, Mob *const m)
{
    FOVCallback fovCallback;
    gameState.b2world->RayCast(&fovCallback, p->body->GetPosition(), m->body->GetPosition());
    return fovCallback.closest->GetBody() == m->body;
}

void makeMobData(Player *const player, flatbuffers::FlatBufferBuilder &builder)
{
    std::vector<flatbuffers::Offset<DeadFish::Mob>> mobs;
    for (auto &n : gameState.civilians)
    {
        if (!playerSeeMob(player, n.get()))
        {
            continue;
        }
        auto posVec = DeadFish::Vec2(n->body->GetPosition().x, n->body->GetPosition().y);
        auto mob = DeadFish::CreateMob(builder,
                                       n->id,
                                       &posVec,
                                       n->body->GetAngle(),
                                       DeadFish::MobState::MobState_Idle,
                                       n->species);
        mobs.push_back(mob);
    }
    for (auto &p : gameState.players)
    {
        if (p.get() != player && !playerSeeMob(player, p.get()))
        {
            continue;
        }
        auto posVec = DeadFish::Vec2(p->body->GetPosition().x, p->body->GetPosition().y);
        auto mob = DeadFish::CreateMob(builder,
                                       p->id,
                                       &posVec,
                                       p->body->GetAngle(),
                                       DeadFish::MobState::MobState_Idle,
                                       p->species);
        mobs.push_back(mob);
    }
    auto mobsOffset = builder.CreateVector(mobs);

    auto worldState = DeadFish::CreateWorldState(builder, mobsOffset);

    auto message = DeadFish::CreateServerMessage(builder,
                                                 DeadFish::ServerMessageUnion_WorldState,
                                                 worldState.Union());

    builder.Finish(message);
}

class TestContactListener : public b2ContactListener
{
    void BeginContact(b2Contact *contact)
    {
        // std::cout << "BEGIN CONTACT\n";
    }

    void EndContact(b2Contact *contact)
    {
        // std::cout << "END CONTACT\n";
    }
};

void physicsInitMob(Mob *m, glm::vec2 pos, float angle, float radius)
{
    b2BodyDef myBodyDef;
    myBodyDef.type = b2_dynamicBody;      //this will be a dynamic body
    myBodyDef.position.Set(pos.x, pos.y); //set the starting position
    myBodyDef.angle = angle;              //set the starting angle
    m->body = gameState.b2world->CreateBody(&myBodyDef);
    b2CircleShape circleShape;
    circleShape.m_radius = radius;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circleShape;
    fixtureDef.density = 1;
    fixtureDef.friction = 0;
    m->body->CreateFixture(&fixtureDef);
    m->body->SetUserData(m);
}

std::vector<int> civiliansSpeciesCount()
{
    std::vector<int> ret;
    ret.resize(gameState.players.size());
    for (auto &c : gameState.civilians)
    {
        ret[c->species]++;
    }
    return ret;
}

void spawnCivilian()
{
    // find spawns
    std::vector<std::string> spawns;
    for (auto &p : gameState.level->navpoints)
    {
        if (p.second->isspawn)
        {
            spawns.push_back(p.first);
        }
    }
    auto &spawnName = spawns[rand() % spawns.size()];
    auto spawn = gameState.level->navpoints[spawnName].get();
    auto c = new Civilian;

    // find species with lowest count of civilians
    auto civCounts = civiliansSpeciesCount();
    int lowestSpecies = 0;
    int lowestSpeciesCount = INT_MAX;
    for (int i = 0; i < civCounts.size(); i++) {
        if (civCounts[i] < lowestSpeciesCount) {
            lowestSpecies = i;
            lowestSpeciesCount = civCounts[i];
        }
    }

    c->id = newID();
    c->species = lowestSpecies;
    std::cout << "spawning species " << lowestSpecies << "\n";
    c->previousNavpoint = spawnName;
    c->currentNavpoint = spawnName;
    physicsInitMob(c, spawn->position, 0, 0.3f);
    c->setNextNavpoint();
    gameState.civilians.push_back(std::unique_ptr<Civilian>(c));
}

void spawnPlayer(Player *const p)
{
    // find spawns
    std::vector<std::string> spawns;
    for (auto &p : gameState.level->navpoints)
    {
        if (p.second->isplayerspawn)
        {
            spawns.push_back(p.first);
        }
    }
    auto &spawnName = spawns[rand() % spawns.size()];
    auto spawn = gameState.level->navpoints[spawnName].get();
    physicsInitMob(p, spawn->position, 0, 0.3f);
    p->targetPosition = spawn->position;
}

void gameThread()
{
    // init physics
    gameState.b2world = std::make_unique<b2World>(b2Vec2(0, 0));
    TestContactListener tcl;
    gameState.b2world->SetContactListener(&tcl);

    // load level
    gameState.level = std::make_unique<Level>();
    std::string path("../../levels/test.bin");
    loadLevel(path);

    // send level to clients
    flatbuffers::FlatBufferBuilder builder(1);
    auto levelOffset = serializeLevel(builder);
    auto message = DeadFish::CreateServerMessage(builder,
                                                 DeadFish::ServerMessageUnion_Level,
                                                 levelOffset.Union());
    builder.Finish(message);
    auto data = builder.GetBufferPointer();
    auto size = builder.GetSize();
    auto str = std::string(data, data + size);
    for (auto &player : gameState.players)
    {
        websocket_server.send(player->conn_hdl, str, websocketpp::frame::opcode::binary);
    }

    for (auto &player : gameState.players)
    {
        spawnPlayer(player.get());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int civilianTimer = 0;

    while (1)
    {
        auto frameStart = std::chrono::system_clock::now();

        // update physics
        gameState.b2world->Step(1 / 20.0, 8, 3);

        // update everyone
        std::vector<int> despawns;
        for (int i = 0; i < gameState.civilians.size(); i++)
        {
            if (!gameState.civilians[i]->update())
            {
                despawns.push_back(i);
            }
        }
        for (int i = despawns.size() - 1; i >= 0; i--)
        {
            std::cout << "despawning civilian\n";
            gameState.civilians.erase(gameState.civilians.begin() + despawns[i]);
        }

        for (auto &p : gameState.players)
        {
            p->update();
        }

        if (civilianTimer == 0 && gameState.civilians.size() < MAX_CIVILIANS)
        {
            spawnCivilian();
            civilianTimer = CIVILIAN_TIME;
        }
        else
        {
            civilianTimer = std::max(0, civilianTimer - 1);
        }

        // send data to everyone
        for (auto &p : gameState.players)
        {
            builder.Clear();
            makeMobData(p.get(), builder);
            auto data = builder.GetBufferPointer();
            str = std::string(data, data + builder.GetSize());
            websocket_server.send(p->conn_hdl, str, websocketpp::frame::opcode::binary);
        }

        // sleep for the remaining of time
        std::this_thread::sleep_until(frameStart + std::chrono::milliseconds(FRAME_TIME));
    }
}