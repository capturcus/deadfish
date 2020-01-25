#include <iostream>
#include <unistd.h>
#include "deadfish.hpp"
#include "game_thread.hpp"

const int FRAME_TIME = 50; // 20 fps

bool operator==(websocketpp::connection_hdl &a, websocketpp::connection_hdl &b)
{
    return a.lock().get() == b.lock().get();
}

uint16_t newID() {
    while (true) {
        uint16_t ret = rand() % UINT16_MAX;

        for (auto& p: gameState.players) {
            if(p->id == ret)
                continue;
        }

        for (auto& n: gameState.npcs) {
            if(n->id == ret)
                continue;
        }

        return ret;
    }
}

Player* getPlayerByConnHdl(websocketpp::connection_hdl& hdl) {
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

void gameOnMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    const auto payload = msg->get_payload();
    const auto clientMessage = flatbuffers::GetRoot<DeadFish::ClientMessage>(payload.c_str());
    switch (clientMessage->event_type())
    {
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandMove:
    {
        const auto event = clientMessage->event_as_CommandMove();
        auto p = getPlayerByConnHdl(hdl);
        p->targetPosition = glm::vec2(event->target()->x(), event->target()->y());
        std::cout << "target: " << p->targetPosition << "\n";
    }
    break;

    default:
        std::cout << "gameOnMessage: some other message type received\n";
        break;
    }
}

void makeMobData(flatbuffers::FlatBufferBuilder& builder) {
    std::vector<flatbuffers::Offset<DeadFish::Mob>> mobs;
    for (auto& n: gameState.npcs) {
        auto posVec = DeadFish::Vec2(n->position.x, n->position.y);
        auto mob = DeadFish::CreateMob(builder,
            n->id,
            &posVec,
            n->angle,
            DeadFish::MobState::MobState_Idle,
            n->species);
        mobs.push_back(mob);
    }
    for (auto& p: gameState.players) {
        auto posVec = DeadFish::Vec2(p->position.x, p->position.y);
        auto mob = DeadFish::CreateMob(builder,
            p->id,
            &posVec,
            p->angle,
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

void gameThread() {
    flatbuffers::FlatBufferBuilder builder(1);
    auto ev = DeadFish::CreateSimpleServerEvent(builder, DeadFish::SimpleServerEventType_GameStart);

    auto message = DeadFish::CreateServerMessage(builder,
        DeadFish::ServerMessageUnion_SimpleServerEvent,
        ev.Union());

    builder.Finish(message);
    auto data = builder.GetBufferPointer();
    auto str = std::string(data, data + builder.GetSize());
    for (auto &player : gameState.players)
    {
        websocket_server.send(player->conn_hdl, str, websocketpp::frame::opcode::binary);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    while(1) {
        auto frameStart = std::chrono::system_clock::now();
        // update everyone
        for (auto& n: gameState.npcs) {
            n->update();
        }
        for (auto& p : gameState.players) {
            p->update();
        }

        // send everything to everyone
        builder.Clear();
        makeMobData(builder);
        data = builder.GetBufferPointer();
        str = std::string(data, data + builder.GetSize());
        for (auto& p : gameState.players) {
            websocket_server.send(p->conn_hdl, str, websocketpp::frame::opcode::binary);
        }

        // sleep for the remaining of time
        std::this_thread::sleep_until(frameStart + std::chrono::milliseconds(FRAME_TIME));
    }
}