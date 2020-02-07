#include <iostream>
#include <limits>
#include <unistd.h>
#include "deadfish.hpp"
#include <glm/gtx/vector_angle.hpp>
#include "game_thread.hpp"
#include "level_loader.hpp"

const int FRAME_TIME = 50; // 20 fps
const int CIVILIAN_TIME = 40;
const int MAX_CIVILIANS = 6;
const float KILL_DISTANCE = 1.f;

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

Player *const getPlayerByConnHdl(websocketpp::connection_hdl &hdl)
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

float revLerp(float min, float max, float val)
{
    if (val < min)
        return 0;
    if (val > max)
        return 1;
    return (val - min) / (max - min);
}

flatbuffers::Offset<DeadFish::Indicator>
makePlayerIndicator(flatbuffers::FlatBufferBuilder &builder,
                    Player *const rootPlayer,
                    Player *const otherPlayer,
                    bool canSeeOther)
{
    glm::vec2 toTarget = b2g(otherPlayer->body->GetPosition()) - b2g(rootPlayer->body->GetPosition());
    float targetAngle = -glm::orientedAngle(glm::normalize(toTarget), glm::vec2(1, 0));
    float force = revLerp(6, 12, glm::length(toTarget));
    return DeadFish::CreateIndicator(builder, targetAngle, force, canSeeOther);
}

void makeWorldState(Player *const player, flatbuffers::FlatBufferBuilder &builder)
{
    std::vector<flatbuffers::Offset<DeadFish::Mob>> mobs;
    std::vector<flatbuffers::Offset<DeadFish::Indicator>> indicators;
    for (auto &n : gameState.civilians)
    {
        if (!playerSeeMob(player, n.get()))
            continue;

        auto posVec = DeadFish::Vec2(n->body->GetPosition().x, n->body->GetPosition().y);
        auto mob = DeadFish::CreateMob(builder,
                                       n->id,
                                       &posVec,
                                       n->body->GetAngle(),
                                       (DeadFish::MobState)n->state,
                                       n->species);
        mobs.push_back(mob);
    }
    for (auto &p : gameState.players)
    {
        bool differentPlayer = p->id != player->id;
        bool canSeeOther;
        if (differentPlayer)
        {
            canSeeOther = playerSeeMob(player, p.get());
            auto indicator = makePlayerIndicator(builder, player, p.get(), canSeeOther);
            indicators.push_back(indicator);
        }
        if (differentPlayer && !canSeeOther)
        {
            continue;
        }
        auto posVec = DeadFish::Vec2(p->body->GetPosition().x, p->body->GetPosition().y);
        auto mob = DeadFish::CreateMob(builder,
                                       p->id,
                                       &posVec,
                                       p->body->GetAngle(),
                                       (DeadFish::MobState)p->state,
                                       p->species);
        mobs.push_back(mob);
    }
    auto mobsOffset = builder.CreateVector(mobs);
    auto indicatorsOffset = builder.CreateVector(indicators);

    auto worldState = DeadFish::CreateWorldState(builder, mobsOffset, indicatorsOffset);

    auto message = DeadFish::CreateServerMessage(builder,
                                                 DeadFish::ServerMessageUnion_WorldState,
                                                 worldState.Union());

    builder.Finish(message);
}

class TestContactListener : public b2ContactListener
{
    void BeginContact(b2Contact *contact)
    {
        auto collideableA = (Collideable *)contact->GetFixtureA()->GetBody()->GetUserData();
        auto collideableB = (Collideable *)contact->GetFixtureB()->GetBody()->GetUserData();

        if (collideableA && !collideableA->toBeDeleted)
            collideableA->handleCollision(collideableB);
        if (collideableB && !collideableB->toBeDeleted)
            collideableB->handleCollision(collideableA);
    }

    void EndContact(b2Contact *contact)
    {
        // std::cout << "END CONTACT\n";
    }
};

void physicsInitMob(Mob *m, glm::vec2 pos, float angle, float radius, uint16 categoryBits = 1)
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
    fixtureDef.filter.categoryBits = categoryBits;
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
    for (int i = 0; i < civCounts.size(); i++)
    {
        if (civCounts[i] < lowestSpeciesCount)
        {
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
    uint64_t maxMinDist = 0;
    std::string maxSpawn = "";
    for (auto &p : gameState.level->navpoints)
    {
        if (p.second->isplayerspawn)
        {
            float minDist = std::numeric_limits<float>::max();
            for (auto &pl : gameState.players)
            {
                if (!pl->body)
                    continue;
                auto dist = b2Distance(g2b(p.second->position), pl->body->GetPosition());
                if (dist < minDist)
                {
                    minDist = dist;
                }
            }
            if (minDist > maxMinDist)
            {
                maxMinDist = minDist;
                maxSpawn = p.first;
            }
        }
    }
    auto spawn = gameState.level->navpoints[maxSpawn].get();
    physicsInitMob(p, spawn->position, 0, 0.3f, 3);
    // physicsInitMob(p, spawn->position, 0, 0.3f);
    p->targetPosition = spawn->position;
}

void executeCommandKill(Player *const player, uint16_t id)
{
    std::cout << "player " << player->name << " trying to kill " << id << "\n";
    Civilian *civ = nullptr;
    for (auto &c : gameState.civilians)
    {
        if (c->id == id)
        {
            civ = c.get();
            break;
        }
    }
    Player *pl = nullptr;
    for (auto &p : gameState.players)
    {
        if (p->id == id)
        {
            pl = p.get();
            break;
        }
    }
    Mob *m = civ ? (Mob *)civ : (Mob *)pl;
    if (b2Distance(m->body->GetPosition(), player->body->GetPosition()) < KILL_DISTANCE)
    {
        player->killTarget = m;
    }
    else
    {
        // send message too far
        flatbuffers::FlatBufferBuilder builder(1);
        auto ev = DeadFish::CreateSimpleServerEvent(builder, DeadFish::SimpleServerEventType_TooFarToKill);
        auto message = DeadFish::CreateServerMessage(builder,
                                                     DeadFish::ServerMessageUnion_SimpleServerEvent,
                                                     ev.Union());
        builder.Finish(message);
        auto data = builder.GetBufferPointer();
        auto size = builder.GetSize();
        auto str = std::string(data, data + size);
        websocket_server.send(player->conn_hdl, str, websocketpp::frame::opcode::binary);
    }
}

void executeKill(Player *p, Mob *m)
{
    p->killTarget = nullptr;
    p->state = MobState::ATTACKING;
    p->attackTimeout = 40;
    // was it a civilian?
    for (auto it = gameState.civilians.begin(); it != gameState.civilians.end(); it++)
    {
        if ((*it)->id == m->id)
        {
            // it was a civ
            (*it)->toBeDeleted = true;
            // send the killednpc message
            flatbuffers::FlatBufferBuilder builder;
            auto ev = DeadFish::CreateSimpleServerEvent(builder, DeadFish::SimpleServerEventType_KilledCivilian);
            auto message = DeadFish::CreateServerMessage(builder, DeadFish::ServerMessageUnion_SimpleServerEvent,
                                                         ev.Union());
            builder.Finish(message);
            auto data = builder.GetBufferPointer();
            auto size = builder.GetSize();
            auto str = std::string(data, data + size);
            websocket_server.send(p->conn_hdl, str, websocketpp::frame::opcode::binary);
            return;
        }
    }

    // was it a player?
    for (auto it = gameState.players.begin(); it != gameState.players.end(); it++)
    {
        if ((*it)->id == m->id)
        {
            // it was a PLAYER
            (*it)->toBeDeleted = true;
            // send the killedplayer message
            flatbuffers::FlatBufferBuilder builder;
            auto name = builder.CreateString((*it)->name);
            auto ev = DeadFish::CreateKilledPlayer(builder, name);
            auto message = DeadFish::CreateServerMessage(builder, DeadFish::ServerMessageUnion_KilledPlayer,
                                                         ev.Union());
            builder.Finish(message);
            auto data = builder.GetBufferPointer();
            auto size = builder.GetSize();
            auto str = std::string(data, data + size);
            websocket_server.send(p->conn_hdl, str, websocketpp::frame::opcode::binary);
            return;
        }
    }
}

void gameOnMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    const auto payload = msg->get_payload();
    const auto clientMessage = flatbuffers::GetRoot<DeadFish::ClientMessage>(payload.c_str());
    auto p = getPlayerByConnHdl(hdl);
    if (p->state == MobState::ATTACKING)
        return;
    switch (clientMessage->event_type())
    {
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandMove:
    {
        const auto event = clientMessage->event_as_CommandMove();
        p->targetPosition = glm::vec2(event->target()->x(), event->target()->y());
        p->state = p->state == MobState::RUNNING ? MobState::RUNNING : MobState::WALKING;
        p->killTarget = nullptr;
    }
    break;
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandRun:
    {
        const auto event = clientMessage->event_as_CommandRun();
        p->state = event->run() ? MobState::RUNNING : MobState::WALKING;
    }
    break;
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandKill:
    {
        const auto event = clientMessage->event_as_CommandKill();
        executeCommandKill(p, event->id());
    }
    break;

    default:
        std::cout << "gameOnMessage: some other message type received\n";
        break;
    }
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
            if (!gameState.civilians[i]->update() || gameState.civilians[i]->toBeDeleted)
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
            makeWorldState(p.get(), builder);
            auto data = builder.GetBufferPointer();
            str = std::string(data, data + builder.GetSize());
            websocket_server.send(p->conn_hdl, str, websocketpp::frame::opcode::binary);
        }

        // sleep for the remaining of time
        std::this_thread::sleep_until(frameStart + std::chrono::milliseconds(FRAME_TIME));
    }
}