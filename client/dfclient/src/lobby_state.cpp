#include <iostream>
#include <functional>

#include <ncine/Application.h>

#include "lobby_state.hpp"
#include "game_data.hpp"
#include "../../../common/deadfish_generated.h"
#include "fb_util.hpp"
#include "state_manager.hpp"
#include "util.hpp"

void LobbyState::OnMessage(const std::string& data) {
    std::cout << "lobby received data\n";
    auto level = FBUtilGetServerEvent(data, Level);
    if (level) {
        gameData.levelData = data; // copy it
        manager.EnterState("gameplay");
        return;
    }
    auto initMetadata = FBUtilGetServerEvent(data, InitMetadata);
    if (!initMetadata) {
        std::cout << "wrong data received\n";
        return;
    }
    gameData.myMobID = initMetadata->yourMobID();
    gameData.myPlayerID = initMetadata->yourPlayerID();
    std::cout << "my mob id " << gameData.myMobID << "\n";
    gameData.players.clear();
    for (size_t i = 0; i < initMetadata->players()->size(); i++)
    {
        auto playerData = initMetadata->players()->Get(i);
        gameData.players.emplace_back();
        gameData.players.back().name = playerData->name()->c_str();
        gameData.players.back().ready = playerData->ready();
        gameData.players.back().species = playerData->species();
        gameData.players.back().playerID = playerData->playerID();
    }
}

void LobbyState::Create() {
    std::cout << "lobby create\n";

    // if we're here then that means that the socket has already connected, send data
    gameData.socket->onMessage = std::bind(&LobbyState::OnMessage, this, std::placeholders::_1);
    flatbuffers::FlatBufferBuilder builder;
    auto req = DeadFish::CreateJoinRequest(builder, builder.CreateString(gameData.myNickname));
    auto message = DeadFish::CreateClientMessage(builder, DeadFish::ClientMessageUnion_JoinRequest, req.Union());
    builder.Finish(message);

    SendData(builder);

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

void LobbyState::SendPlayerReady() {
    flatbuffers::FlatBufferBuilder builder;
    auto ready = DeadFish::CreatePlayerReady(builder);
    auto message = DeadFish::CreateClientMessage(builder, DeadFish::ClientMessageUnion_PlayerReady, ready.Union());
    builder.Finish(message);

    SendData(builder);
}

void LobbyState::OnMouseButtonPressed(const ncine::MouseEvent &event) {
    if(IntersectsNode(event.x, event.y, *this->readyButton) && !this->ready) {
        this->ready = true;
        this->SendPlayerReady();
    }
}
