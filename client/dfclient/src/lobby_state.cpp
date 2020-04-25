#include <iostream>

#include <ncine/Application.h>

#include "lobby_state.hpp"
#include "game_data.hpp"
#include "../../../common/deadfish_generated.h"
#include "fb_util.hpp"
#include "state_manager.hpp"

// extern StateManager stateManager;

void LobbyOnMessage(std::string& data) {
    std::cout << "lobby received data\n";
    auto initMetadata = FBUtilGetServerEvent(data, InitMetadata);
    gameData.myID = initMetadata->yourid();
    gameData.players.clear();
    for (size_t i = 0; i < initMetadata->players()->size(); i++)
    {
        auto playerData = initMetadata->players()->Get(i);
        gameData.players.emplace_back();
        gameData.players.back().name = playerData->name()->c_str();
        gameData.players.back().ready = playerData->ready();
        gameData.players.back().species = playerData->species();
    }

    // ((LobbyState*)stateManager.states["lobby"].get())->RedrawPlayers();
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

    auto& rootNode = ncine::theApplication().rootNode();
    const float screenWidth = ncine::theApplication().width();
    const float screenHeight = ncine::theApplication().height();

    auto text = std::make_unique<ncine::TextNode>(&rootNode, this->manager.fonts["comic"].get());
	text->setScale(1.0f);
	text->setString("players in lobby:");
	text->setPosition(screenWidth * 0.3f, screenHeight * 0.8f);
	text->setColor(0, 0, 0, 255);
    this->sceneNodes.push_back(std::move(text));

    this->readyButton = std::make_unique<ncine::TextNode>(&rootNode, this->manager.fonts["comic"].get());
	this->readyButton->setScale(1.0f);
	this->readyButton->setString("READY");
	this->readyButton->setPosition(screenWidth * 0.5f, screenHeight * 0.2f);
}

void LobbyState::Update() {
    RedrawPlayers();
    if (this->ready)
        this->readyButton->setColor(128, 128, 128, 255);
    else
        this->readyButton->setColor(100, 0, 0, 255);
}

void LobbyState::CleanUp() {
    this->sceneNodes.clear();
    this->textNodes.clear();
    this->readyButton.reset(nullptr);
}

void LobbyState::RedrawPlayers() {
    this->textNodes.clear();
    uint16_t playerNum = 0;
    auto& rootNode = ncine::theApplication().rootNode();
    const float screenWidth = ncine::theApplication().width();
    const float screenHeight = ncine::theApplication().height();
    for (auto& p : gameData.players) {
        auto text = std::make_unique<ncine::TextNode>(&rootNode, this->manager.fonts["comic"].get());
        text->setScale(1.0f);
        text->setString(p.name.c_str());
        text->setPosition(screenWidth * 0.3f, screenHeight * 0.75f - 40*playerNum);
        if (p.ready)
            text->setColor(0, 128, 0, 255);
        else
            text->setColor(0, 0, 0, 255);
        this->textNodes.push_back(std::move(text));
        playerNum++;
    }
}