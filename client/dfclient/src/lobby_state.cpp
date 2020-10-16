#include <iostream>
#include <functional>

#include <ncine/Application.h>

#include "lobby_state.hpp"
#include "game_data.hpp"
#include "../../../common/deadfish_generated.h"
#include "fb_util.hpp"
#include "resources.hpp"
#include "util.hpp"

StateType LobbyState::OnMessage(const std::string& data) {
	std::cout << "lobby received data\n";
	auto event = FBUtilGetServerEvent(data, SimpleServerEvent);
	if (event) {
		if (event->type() == FlatBuffGenerated::SimpleServerEventType_GameAlreadyInProgress) {
			std::cout << "game already in progress\n";
			gameData.gameInProgress = true;
			return StateType::Menu;
		}
	}
	auto level = FBUtilGetServerEvent(data, Level);
	if (level) {
		gameData.levelData = data; // copy it
		return StateType::Gameplay;
	}
	auto initMetadata = FBUtilGetServerEvent(data, InitMetadata);
	if (!initMetadata) {
		std::cout << "wrong data received\n";
		return StateType::Lobby;
	}
	gameData.myMobID = initMetadata->yourMobID();
	gameData.myPlayerID = initMetadata->yourPlayerID();
	std::cout << "my mob id " << gameData.myMobID << "\n";
	gameData.players.clear();
	for (size_t i = 0; i < initMetadata->players()->size(); i++)
	{
		auto playerData = initMetadata->players()->Get(i);
		gameData.players.emplace_back();
		gameData.players.back().name = playerData->name()->str();
		gameData.players.back().ready = playerData->ready();
		gameData.players.back().species = playerData->species();
		gameData.players.back().playerID = playerData->playerID();
	}

	return StateType::Lobby;
}

LobbyState::LobbyState(Resources& r) : _resources(r) {
	std::cout << "lobby create\n";

	flatbuffers::FlatBufferBuilder builder;
	auto req = FlatBuffGenerated::CreateJoinRequest(builder, builder.CreateString(gameData.myNickname));
	auto message = FlatBuffGenerated::CreateClientMessage(builder, FlatBuffGenerated::ClientMessageUnion_JoinRequest, req.Union());
	builder.Finish(message);

	SendData(builder);

	auto& rootNode = ncine::theApplication().rootNode();
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();

	auto text = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());
	text->setScale(1.0f);
	text->setString("players in lobby:");
	text->setPosition(screenWidth * 0.3f, screenHeight * 0.8f);
	text->setColor(0, 0, 0, 255);
	this->sceneNodes.push_back(std::move(text));

	this->readyButton = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());
	this->readyButton->setScale(1.0f);
	this->readyButton->setString("READY");
	this->readyButton->setPosition(screenWidth * 0.5f, screenHeight * 0.2f);
}

StateType LobbyState::Update(Messages m) {
	for (auto& msg: m.data_msgs) {
		auto msgState = OnMessage(msg);
		if (msgState != StateType::Lobby) {
			return msgState;
		}
	}

	RedrawPlayers();
	if (this->ready)
		this->readyButton->setColor(128, 128, 128, 255);
	else
		this->readyButton->setColor(100, 0, 0, 255);

	return StateType::Lobby;
}

LobbyState::~LobbyState() {
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
		auto text = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());
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
	auto ready = FlatBuffGenerated::CreatePlayerReady(builder);
	auto message = FlatBuffGenerated::CreateClientMessage(builder, FlatBuffGenerated::ClientMessageUnion_PlayerReady, ready.Union());
	builder.Finish(message);

	SendData(builder);
}

void LobbyState::OnMouseButtonPressed(const ncine::MouseEvent &event) {
	if (IntersectsNode(event.x, event.y, *this->readyButton) && !this->ready) {
		this->ready = true;
		this->SendPlayerReady();
	}
}
