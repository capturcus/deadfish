#include <functional>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
	_resources._tweens.push_back(CreateTextTween(text));
}

MenuState::MenuState(Resources& r) : _resources(r) {
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	auto res = nc::theApplication().appConfiguration().resolution;
	logoSprite = nctl::makeUnique<nc::Sprite>(&rootNode, _resources.textures["deadfish.png"].get(), res.x*0.5f, res.y*0.6f);
	nc::theApplication().gfxDevice().setClearColor(bgColor);
	if (gameData.gameInProgress) {
		std::cout << "menu game in progress\n";
		ShowMessage("game already in progress");
		gameData.gameInProgress = false;
	}
}

bool MenuState::TryConnect() {
	gameData.serverAddress = "ws://" + gameData.serverAddress;
	std::cout << "server " << gameData.serverAddress << ", my nickname " << gameData.myNickname << "\n";
	gameData.socket = CreateWebSocket();
	int ret = gameData.socket->Connect(gameData.serverAddress);
	if (ret < 0) {
		std::cout << "socket->Connect failed " << ret << "\n";
		// TODO: some ui error handling
		return false;
	}
	return true;
}

void MenuState::ProcessMatchmakerData(std::string data) {
	boost::property_tree::ptree tree;
	std::stringstream ss;
	ss << data;
	boost::property_tree::read_json(ss, tree);

	auto status = tree.get("status", "");
	auto address = tree.get("address", "");
	auto port = tree.get("port", 0);
	std::cout << "status: " << status << ", address: " << address << ", port: " << port << "\n";
	gameData.serverAddress = address + ":" + std::to_string(port);
	this->TryConnect();
}

StateType MenuState::Update(Messages m) {
	if (m.opened) {
		return StateType::Lobby;
	}

	ImGui::Begin("DeadFish", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowSize({320, 100});
	auto res = nc::theApplication().appConfiguration().resolution;
	ImGui::SetWindowPos({static_cast<float>(res.x)/2-175, 4*static_cast<float>(res.y)/5});
	static char buf[64] = "";
	ImGui::InputText("nickname", buf, 64);
	if (ImGui::Button("find game", {300, 30})) {
		ImGui::End();
		gameData.myNickname = std::string(buf);
		if (gameData.myNickname.empty()) {
			this->ShowMessage("enter a nickname");
			return StateType::Menu;
		}
		std::string matchmakerData;
		int ret = http::MatchmakerGet(matchmakerData);
		if (ret < 0)
			this->ShowMessage("couldn't connect to the matchmaker");
		std::cout << "ret " << ret << "\n";
		if (ret > 0)
			this->ProcessMatchmakerData(matchmakerData);
	} else
		ImGui::End();

	return StateType::Menu;
}

MenuState::~MenuState() {
	logoSprite.reset(nullptr);
}
