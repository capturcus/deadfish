#include <functional>
#include <iostream>

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
#include "text_creator.hpp"

namespace nc = ncine;

nctl::UniquePtr<nc::Sprite> logoSprite;
nc::Colorf bgColor(0.96875f, 0.97265625, 0.953125, 1.0f);

MenuState::MenuState(Resources& r) : _resources(r) {
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	auto res = nc::theApplication().appConfiguration().resolution;
	logoSprite = nctl::makeUnique<nc::Sprite>(&rootNode, _resources.textures["deadfish.png"].get(), res.x*0.5f, res.y*0.6f);
	nc::theApplication().gfxDevice().setClearColor(bgColor);
	if (gameData.gameInProgress) {
		std::cout << "menu game in progress\n";
		auto text = new ncine::TextNode(&rootNode, _resources.fonts["comic"].get());
		text->setString("game already in progress");
		text->setPosition(res.x * 0.5f, res.y * 0.75f);
		text->setScale(2.0f);
		text->setColor(0, 0, 0, 255);
		_resources._intTweens.push_back(textCreator->CreateTextTween(text));
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

StateType MenuState::Update(Messages m) {
	if (m.opened) {
		return StateType::Lobby;
	}

	ImGui::Begin("DeadFish", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowSize({350, 120});
	auto res = nc::theApplication().appConfiguration().resolution;
	ImGui::SetWindowPos({static_cast<float>(res.x)/2-175, 4*static_cast<float>(res.y)/5});
	static char buf1[64] = "localhost:63987";
	static char buf2[64] = "asd";
	ImGui::InputText("server", buf1, 64);
	ImGui::InputText("nickname", buf2, 64);
	if (ImGui::Button("connect", {300, 30})) {
		gameData.serverAddress = std::string(buf1);
		gameData.myNickname = std::string(buf2);
		TryConnect();
	}
	ImGui::End();

	return StateType::Menu;
}

MenuState::~MenuState() {
	logoSprite.reset(nullptr);
}
