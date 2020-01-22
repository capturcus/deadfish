#include <iostream>
#include <unordered_map>

#include "flatbuffers/flatbuffers.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "deadfish_generated.h"
#include "game.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

GameState gameState;
server websocket_server;

void sendInitMetadata()
{
    for (auto &targetPlayer : gameState.players)
    {
        flatbuffers::FlatBufferBuilder builder(1024);
        std::vector<flatbuffers::Offset<DeadFish::InitPlayer>> playerOffsets;
        for (auto &player : gameState.players)
        {
            auto name = builder.CreateString(player.second->name.c_str());
            auto playerOffset = DeadFish::CreateInitPlayer(builder, player.second->id, name, player.second->species);
            playerOffsets.push_back(playerOffset);
        }
        auto players = builder.CreateVector(playerOffsets);
        auto metadata = DeadFish::CreateInitMetadata(builder, 0, players, targetPlayer.second->id);

        auto message = DeadFish::CreateServerMessage(builder,
            DeadFish::ServerMessageUnion::ServerMessageUnion_InitMetadata,
            metadata.Union());

        builder.Finish(message);
        auto data = builder.GetBufferPointer();

        websocket_server.send(targetPlayer.second->conn_hdl, std::string(data, data + builder.GetSize()), websocketpp::frame::opcode::binary);
    }
}

void addNewPlayer(const std::string &name, websocketpp::connection_hdl hdl)
{
    if (gameState.phase != GamePhase::LOBBY)
    {
        return;
    }

    auto p = new Player();
    p->id = ++gameState.lastPlayerID;
    auto addedID = p->id;
    p->name = name;
    p->conn_hdl = hdl;
    p->species = 0;

    gameState.players.insert({p->id, std::unique_ptr<Player>(p)});

    sendInitMetadata();
}

void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    const auto payload = msg->get_payload();
    const auto clientMessage = flatbuffers::GetRoot<DeadFish::ClientMessage>(payload.c_str());
    switch (clientMessage->event_type())
    {
    case DeadFish::ClientMessageUnion::ClientMessageUnion_CommandMove:
    {
        const auto event = clientMessage->event_as_CommandMove();
        std::cout << event->target()->x() << ", " << event->target()->y() << std::endl;
    }
    break;

    case DeadFish::ClientMessageUnion::ClientMessageUnion_JoinRequest:
    {
        const auto event = clientMessage->event_as_JoinRequest();
        std::cout << "new player " << event->name()->c_str() << "\n";
        addNewPlayer(event->name()->c_str(), hdl);
        std::cout << "player count " << gameState.players.size() << "\n";
    }
    break;

    default:
        break;
    }
}

bool operator==(websocketpp::connection_hdl& a, websocketpp::connection_hdl& b) {
    return a.lock().get() == b.lock().get();
}

void on_close(websocketpp::connection_hdl hdl) {
    uint16_t toBeDeleted = -1;
    for (auto &player : gameState.players)
    {
        if (player.second->conn_hdl == hdl) {
            std::cout << "deleting player " << player.second->name << "\n";
            toBeDeleted = player.second->id;
        }
    }
    gameState.players.erase(toBeDeleted);
}

int main()
{

    websocket_server.set_message_handler(&on_message);
    websocket_server.set_reuse_addr(true);
    websocket_server.set_close_handler(&on_close);

    websocket_server.init_asio();
    websocket_server.listen(63987);
    websocket_server.start_accept();

    std::cout << "server started\n";

    websocket_server.run();
}