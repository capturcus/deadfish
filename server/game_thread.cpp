#include <iostream>
#include <limits>
#include "deadfish.hpp"
#include <glm/gtx/vector_angle.hpp>
#include "game_thread.hpp"
#include "level_loader.hpp"

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

std::string makeServerMessage(flatbuffers::FlatBufferBuilder &builder,
                              DeadFish::ServerMessageUnion type,
                              flatbuffers::Offset<void> offset)
{
    auto message = DeadFish::CreateServerMessage(builder,
                                                 type,
                                                 offset);
    builder.Finish(message);
    auto data = builder.GetBufferPointer();
    auto size = builder.GetSize();
    auto str = std::string(data, data + size);
    return str;
}

void sendServerMessage(Player &player,
                       flatbuffers::FlatBufferBuilder &builder,
                       DeadFish::ServerMessageUnion type,
                       flatbuffers::Offset<void> offset)
{
    auto str = makeServerMessage(builder, type, offset);
    websocket_server.send(player.conn_hdl, str, websocketpp::frame::opcode::binary);
}

void sendToAll(std::string &data)
{
    for (auto &p : gameState.players)
    {
        websocket_server.send(p->conn_hdl, data, websocketpp::frame::opcode::binary);
    }
}

void stopAll()
{
    websocket_server.stop_perpetual();
    for (auto& p : gameState.players)
    {
        websocket_server.close(p->conn_hdl, websocketpp::close::status::normal, "goodbye");
    }
}

Player &getPlayerByConnHdl(websocketpp::connection_hdl &hdl)
{
    auto player = gameState.players.begin();
    while (player != gameState.players.end())
    {
        if ((*player)->conn_hdl == hdl)
        {
            return *(player->get());
        }
        player++;
    }
    std::cout << "getPlayerByConnHdl PLAYER NOT FOUND\n";
    exit(1);
}

struct FOVCallback
    : public b2RayCastCallback
{
    float32 ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float32 fraction)
    {
        auto data = (Collideable *)fixture->GetBody()->GetUserData();
        if (data && data != target && !data->obstructsSight())
            return 1.f;
        if (fraction < minfraction)
        {
            minfraction = fraction;
            closest = fixture;
        }
        return fraction;
    }
    float minfraction = 1.f;
    b2Fixture *closest = nullptr;
    Mob *target = nullptr;
};

bool playerSeeMob(Player &p, Mob &m)
{
    if (m.isDead())
        return false; // can't see dead ppl lol
    FOVCallback fovCallback;
    fovCallback.target = &m;
    auto ppos = p.deathTimeout > 0 ? g2b(p.targetPosition) : p.body->GetPosition();
    auto mpos = m.body->GetPosition();
    gameState.b2world->RayCast(&fovCallback, ppos, mpos);
    return fovCallback.closest && fovCallback.closest->GetBody() == m.body;
}

bool mobSeePoint(Mob &m, b2Vec2 &point) {
    FOVCallback fovCallback;
    gameState.b2world->RayCast(&fovCallback, m.body->GetPosition(), point);
    return fovCallback.minfraction == 1.f;
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
                    Player &rootPlayer,
                    Player &otherPlayer)
{
    glm::vec2 toTarget = b2g(otherPlayer.body->GetPosition()) - b2g(rootPlayer.body->GetPosition());
    float force = revLerp(6, 12, glm::length(toTarget));
    return DeadFish::CreateIndicator(builder, 0, force, false);
}

flatbuffers::Offset<void> makeWorldState(Player &player, flatbuffers::FlatBufferBuilder &builder)
{
    std::vector<flatbuffers::Offset<DeadFish::Mob>> mobs;
    std::vector<flatbuffers::Offset<DeadFish::Indicator>> indicators;
    for (auto &n : gameState.civilians)
    {
        // std::cout << "player " << player.name << " pos " << player.body->GetPosition() << " civilian pos " << n->body->GetPosition() << "\n";
        if (!playerSeeMob(player, *n.get()))
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
        if (p->deathTimeout > 0)
        {
            // is dead, don't send
            continue;
        }
        bool differentPlayer = p->id != player.id;
        bool canSeeOther;
        if (differentPlayer)
        {
            canSeeOther = playerSeeMob(player, *p.get());
            auto indicator = makePlayerIndicator(builder, player, *p.get());
            indicators.push_back(indicator);
        }
        if (differentPlayer && !canSeeOther)
            continue;
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

    return worldState.Union();
}

class TestContactListener : public b2ContactListener
{
    void BeginContact(b2Contact *contact)
    {
        auto collideableA = (Collideable *)contact->GetFixtureA()->GetBody()->GetUserData();
        auto collideableB = (Collideable *)contact->GetFixtureB()->GetBody()->GetUserData();

        if (collideableA && !collideableA->toBeDeleted &&
            collideableB && !collideableB->toBeDeleted)
        {
            collideableA->handleCollision(*collideableB);
            collideableB->handleCollision(*collideableA);
        }
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
    auto c = std::make_unique<Civilian>();

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
    c->previousNavpoint = spawnName;
    c->currentNavpoint = spawnName;
    physicsInitMob(c.get(), spawn->position, 0, 0.3f);
    c->setNextNavpoint();
    gameState.civilians.push_back(std::move(c));
    std::cout << "spawning civilian of species " << lowestSpecies <<
        " at " << spawnName << " to a total of " << gameState.civilians.size() << "\n";
}

void spawnPlayer(Player &player)
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
    physicsInitMob(&player, spawn->position, 0, 0.3f, 3);
    player.targetPosition = spawn->position;

    std::cout << "spawned player at " << maxSpawn << ", " << player.body->GetPosition() << "\n";
}

Mob &findMobById(uint16_t id)
{
    auto it = std::find_if(gameState.civilians.begin(), gameState.civilians.end(),
                           [id](const auto &c) { return c->id == id; });
    if (it != gameState.civilians.end())
        return *(*it);

    auto it2 = std::find_if(gameState.players.begin(), gameState.players.end(),
                            [id](const auto &p) { return p->id == id; });
    if (it2 != gameState.players.end())
        return *(*it2);
    std::cout << "could not find mob by id " << id << "\n";
    exit(1);
}

void executeCommandKill(Player &player, uint16_t id)
{
    player.lastAttack = std::chrono::system_clock::now();
    auto &m = findMobById(id);
    auto distance = b2Distance(m.body->GetPosition(), player.body->GetPosition());
    if (distance < INSTA_KILL_DISTANCE)
    {
        player.setAttacking();
        m.handleKill(player);
        return;
    }
    if (distance < KILL_DISTANCE)
    {
        player.killTarget = &m;
        return;
    }
    // send message too far
    flatbuffers::FlatBufferBuilder builder(1);
    auto ev = DeadFish::CreateSimpleServerEvent(builder, DeadFish::SimpleServerEventType_TooFarToKill);
    sendServerMessage(player, builder, DeadFish::ServerMessageUnion_SimpleServerEvent, ev.Union());
}

void sendHighscores()
{
    flatbuffers::FlatBufferBuilder builder;
    std::vector<flatbuffers::Offset<DeadFish::HighscoreEntry>> entries;
    for (auto &p : gameState.players)
    {
        auto entry = DeadFish::CreateHighscoreEntry(builder, builder.CreateString(p->name), p->points);
        entries.push_back(entry);
    }
    auto v = builder.CreateVector(entries);
    auto update = DeadFish::CreateHighscoreUpdate(builder, v);
    auto data = makeServerMessage(builder, DeadFish::ServerMessageUnion_HighscoreUpdate, update.Union());
    sendToAll(data);
}

void gameOnMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    const auto payload = msg->get_payload();
    const auto clientMessage = flatbuffers::GetRoot<DeadFish::ClientMessage>(payload.c_str());
    const auto guard = gameState.lock();

    auto &p = getPlayerByConnHdl(hdl);
    if (p.state == MobState::ATTACKING)
        return;

    switch (clientMessage->event_type())
    {
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandMove:
    {
        const auto event = clientMessage->event_as_CommandMove();
        p.targetPosition = glm::vec2(event->target()->x(), event->target()->y());
        p.state = p.state == MobState::RUNNING ? MobState::RUNNING : MobState::WALKING;
        p.killTarget = nullptr;
        p.lastAttack = std::chrono::system_clock::from_time_t(0);
    }
    break;
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandRun:
    {
        const auto event = clientMessage->event_as_CommandRun();
        p.state = event->run() ? MobState::RUNNING : MobState::WALKING;
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
    TestContactListener tcl;
    flatbuffers::FlatBufferBuilder builder(1);

    {
        const auto guard = gameState.lock();

        // init physics
        gameState.b2world = std::make_unique<b2World>(b2Vec2(0, 0));
        gameState.b2world->SetContactListener(&tcl);

        // load level
        gameState.level = std::make_unique<Level>();
        auto path = gameState.config.get<std::string>("default.level");
        loadLevel(path);

        // send level to clients
        auto levelOffset = serializeLevel(builder);
        auto data = makeServerMessage(builder, DeadFish::ServerMessageUnion_Level, levelOffset.Union());
        sendToAll(data);

        // shuffle the players to give them random species
        std::random_shuffle(gameState.players.begin(), gameState.players.end());

        uint8_t lastSpecies = 0;
        for (auto &player : gameState.players)
        {
            player->species = lastSpecies;
            lastSpecies++;
            spawnPlayer(*player.get());
        }
    }

    int civilianTimer = 0;
    uint64_t roundTimer = ROUND_LENGTH;

    while (1)
    {
        auto maybe_guard = gameState.lock();
        auto frameStart = std::chrono::system_clock::now();

        roundTimer--;
        if (roundTimer == 0)
        {
            // send game end message to everyone
            builder.Clear();
            auto ev = DeadFish::CreateSimpleServerEvent(builder, DeadFish::SimpleServerEventType_GameEnded);
            auto data = makeServerMessage(builder, DeadFish::ServerMessageUnion_SimpleServerEvent, ev.Union());
            sendToAll(data);
            // FIXME: Proper closing of all connections, so that this sleep is unnecessary
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            exit(0);
        }

        // update physics
        gameState.b2world->Step(1 / 20.0, 8, 3);

        // update everyone
        std::vector<int> despawns;
        for (int i = 0; i < gameState.civilians.size(); i++)
        {
            gameState.civilians[i]->update();
            if (gameState.civilians[i]->toBeDeleted)
                despawns.push_back(i);
        }
        for (int i = despawns.size() - 1; i >= 0; i--)
        {
            // std::cout << "despawning civilian\n";
            gameState.civilians.erase(gameState.civilians.begin() + despawns[i]);
        }

        for (auto &p : gameState.players)
            p->update();

        // spawn civilians if need be
        if (civilianTimer == 0 && gameState.civilians.size() < MAX_CIVILIANS)
        {
            spawnCivilian();
            civilianTimer = CIVILIAN_TIME;
        }
        else
            civilianTimer = std::max(0, civilianTimer - 1);

        // send data to everyone
        for (auto &p : gameState.players)
        {
            builder.Clear();
            auto offset = makeWorldState(*p.get(), builder);
            sendServerMessage(*p.get(), builder, DeadFish::ServerMessageUnion_WorldState, offset);
        }

        // Drop the lock
        maybe_guard.reset();

        // sleep for the remaining of time
        std::this_thread::sleep_until(frameStart + std::chrono::milliseconds(FRAME_TIME));
    }
}