#include <iostream>

#include "lobbystate.hpp"
#include "gamedata.hpp"
#include "../../../common/deadfish_generated.h"
#include "fbutil.hpp"

void LobbyOnMessage(std::string& data) {
    std::cout << "lobby received data\n";
    auto initMetadata = FBUtilGetServerEvent(data, InitMetadata);
    gameData.myID = initMetadata->yourid();
    std::cout << "my id " << gameData.myID << "\n";
    for (size_t i = 0; i < initMetadata->players()->size(); i++)
    {
        auto player = initMetadata->players()->Get(i);
        std::cout << "player " << player->name()->c_str() << " " << player->ready() << " " << player->species() << "\n";
    }
}

void LobbyOnOpen() {
    std::cout << "lobby on open\n";
    flatbuffers::FlatBufferBuilder builder;
    auto req = DeadFish::CreateJoinRequest(builder, builder.CreateString(gameData.myNickname));
    auto message = DeadFish::CreateClientMessage(builder, DeadFish::ClientMessageUnion_JoinRequest, req.Union());
    builder.Finish(message);

    auto data = builder.GetBufferPointer();
    auto size = builder.GetSize();
    auto str = std::string(data, data + size);

    gameData.socket->Send(str);
}

void LobbyState::Create() {
    std::cout << "lobby create\n";
    gameData.serverAddress = "ws://" + gameData.serverAddress;
    std::cout << "server " << gameData.serverAddress << ", my nickname " << gameData.myNickname << "\n";
    gameData.socket = CreateWebSocket();
    gameData.socket->onMessage = &LobbyOnMessage;
    gameData.socket->onOpen = &LobbyOnOpen;
    int ret = gameData.socket->Connect(gameData.serverAddress);
    if (ret < 0) {
        std::cout << "socket->Connect failed " << ret << "\n";
        // TODO: some ui error handling
        return;
    } else
        std::cout << "socket connected\n";
}

void LobbyState::Update() {

}

void LobbyState::CleanUp() {

}