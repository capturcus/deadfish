#include <functional>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <ncine/Application.h>
#include <ncine/imgui.h>
#include <ncine/MeshSprite.h>
#include <ncine/SceneNode.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>

#include "fb_util.hpp"
#include "game_data.hpp"
#include "game_data.hpp"
#include "menu_state.hpp"
#include "resources.hpp"
#include "util.hpp"
#include "http_client.hpp"
#include "text_creator.hpp"

namespace nc = ncine;

nctl::UniquePtr<nc::Sprite> logoSprite;
nc::Colorf bgColor(0.96875f, 0.97265625, 0.953125, 1.0f);

void MenuState::ShowMessage(std::string message)
{
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	auto res = nc::theApplication().appConfiguration().resolution;
	auto text = new ncine::TextNode(&rootNode, _resources.fonts["comic"].get());
	text->setString(message.c_str());
	text->setPosition(res.x * 0.5f, res.y * 0.30f);
	text->setScale(0.5f);
	text->setColor(0, 0, 0, 255);
	auto tween = tweeny::from(255)
		.to(255).during(60)
		.to(0).during(60).onStep(
		[text] (tweeny::tween<int>& t, int v) -> bool {
			auto textColor = text->color();
			text->setColor(textColor.r(), textColor.g(), textColor.b(), v);
			std::cout << "v " << v << "\n";
			if (v == 0)
				delete text;
			return false;
		}
	);
	_resources._tweens.push_back(tween);
}

MenuState::MenuState(Resources& r) : _resources(r) {
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	auto res = nc::theApplication().appConfiguration().resolution;

	logoSprite = nctl::makeUnique<nc::Sprite>(&rootNode, _resources.textures["deadfish.png"].get(), res.x*0.5f, res.y*0.6f);
	nc::theApplication().gfxDevice().setClearColor(bgColor);

	if (gameData.gameInProgress) {
		TextCreator textCreator(r);
		std::cout << "menu game in progress\n";
		ShowMessage("game already in progress");
		gameData.gameInProgress = false;
	}
	boost::property_tree::ptree pt;
	boost::property_tree::ini_parser::read_ini("deadfish.ini", pt);
	try {
		this->matchmakerAddress = pt.get<std::string>("default.matchmaker_address", "");
		this->matchmakerPort = pt.get<std::string>("default.matchmaker_port", "");
		this->directAddress = pt.get<std::string>("default.direct_address", "");
		this->directPort = pt.get<std::string>("default.direct_port", "");
		std::string mode = pt.get<std::string>("default.client_mode");
		if (mode == "matchmaker")
			this->clientMode = ClientMode::Matchmaker;
		else if (mode == "direct")
			this->clientMode = ClientMode::DirectConnect;
		else {
			throw std::runtime_error(("unknown client mode: " + mode).c_str());
		}
	} catch (const std::exception& e) {
		std::cout << "failed to read and parse deadfish.ini: " << e.what() << "\n";
		exit(1);
	}
	if (this->clientMode == ClientMode::DirectConnect) {
		auto addr = this->directAddress + ":" + this->directPort;
		strcpy(this->serverAddressBuffer, addr.c_str());
	}
}

void MenuState::TryConnect() {
	gameData.serverAddress = "ws://" + gameData.serverAddress;
	std::cout << "server " << gameData.serverAddress << ", my nickname " << gameData.myNickname << "\n";
	gameData.socket = CreateWebSocket();
	int ret = gameData.socket->Connect(gameData.serverAddress);
	if (ret < 0) {
		std::cout << "socket->Connect failed " << ret << "\n";
		this->ShowMessage("connecting to server failed");
	}
}

void MenuState::ProcessMatchmakerData(std::string data) {
	boost::property_tree::ptree tree;
	std::stringstream ss;
	ss << data;
	boost::property_tree::read_json(ss, tree);

	auto status = tree.get("status", "");
	auto address = tree.get("address", "");
	auto port = tree.get("port", 0);
	if (status == "error") {
		this->ShowMessage("matchmaker error: " + tree.get("error", "unknown error"));
		return;
	}
	std::cout << "status: " << status << ", address: " << address << ", port: " << port << "\n";
	gameData.serverAddress = address + ":" + std::to_string(port);
	this->TryConnect();
}

void MenuState::MatchmakerLayout() {
	ImGui::InputText("nickname", this->nicknameBuffer, 64);
	if (ImGui::Button("find game", {300, 30})) {
		ImGui::End();
		gameData.myNickname = std::string(this->nicknameBuffer);
		if (gameData.myNickname.empty()) {
			this->ShowMessage("enter a nickname");
			return;
		}
		std::string matchmakerData;
		int ret = http::MatchmakerGet(this->matchmakerAddress,
			this->matchmakerPort, matchmakerData);
		if (ret < 0)
			this->ShowMessage("couldn't connect to the matchmaker");
		std::cout << "ret " << ret << "\n";
		if (ret > 0)
			this->ProcessMatchmakerData(matchmakerData);
	} else
		ImGui::End();
}

void MenuState::DirectConnectLayout() {
	ImGui::InputText("server", this->serverAddressBuffer, 64);
	ImGui::InputText("nickname", this->nicknameBuffer, 64);
	if (ImGui::Button("connect", {300, 30})) {
		gameData.serverAddress = std::string(this->serverAddressBuffer);
		gameData.myNickname = std::string(this->nicknameBuffer);
		TryConnect();
	}
	ImGui::End();
}

StateType MenuState::Update(Messages m) {
	if (m.opened) {
		return StateType::Lobby;
	}

	ImGui::Begin("DeadFish", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse);
	float sizeY = this->clientMode == ClientMode::Matchmaker ? 100.f : 120.f;
	ImGui::SetWindowSize({350, sizeY});
	auto res = nc::theApplication().appConfiguration().resolution;
	ImGui::SetWindowPos({static_cast<float>(res.x)/2-175, 4*static_cast<float>(res.y)/5});
	switch (this->clientMode) {
		case ClientMode::Matchmaker:
			this->MatchmakerLayout();
			break;
		case ClientMode::DirectConnect:
			this->DirectConnectLayout();
	}
	

	return StateType::Menu;
}

MenuState::~MenuState() {
	logoSprite.reset(nullptr);
}
