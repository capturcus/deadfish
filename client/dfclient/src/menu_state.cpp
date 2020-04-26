#include <iostream>
#include <functional>

#include <ncine/Application.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>
#include <ncine/imgui.h>

#include "menu_state.hpp"
#include "state_manager.hpp"
#include "game_data.hpp"

namespace nc = ncine;

nctl::UniquePtr<nc::Texture> logo;
nctl::UniquePtr<nc::Sprite> logoSprite;
nc::Colorf bgColor(0.96875f, 0.97265625, 0.953125, 1.0f);

void MenuState::Create() {
    nc::SceneNode &rootNode = nc::theApplication().rootNode();
	// make sprites and stuff
	logo = nctl::makeUnique<nc::Texture>("/deadfish.png");
	auto res = nc::theApplication().appConfiguration().resolution;
	logoSprite = nctl::makeUnique<nc::Sprite>(&rootNode, logo.get(), res.x, res.y);
	logoSprite->setPosition(logoSprite->position() + ncine::Vector2f{-logoSprite->width()/2, 0});

	nc::theApplication().gfxDevice().setClearColor(bgColor);
}

bool MenuState::TryConnect() {
	gameData.serverAddress = "ws://" + gameData.serverAddress;
    std::cout << "server " << gameData.serverAddress << ", my nickname " << gameData.myNickname << "\n";
    gameData.socket = CreateWebSocket();
	StateManager& forwardManager = this->manager;
    gameData.socket->onOpen = [this](){
		std::cout << "lambda go to lobby\n";
		this->manager.EnterState("lobby");
	};
    int ret = gameData.socket->Connect(gameData.serverAddress);
    if (ret < 0) {
        std::cout << "socket->Connect failed " << ret << "\n";
        // TODO: some ui error handling
        return false;
    }
	std::cout << "socket connected\n";
	return true;
}

void MenuState::Update() {
    ImGui::Begin("DeadFish", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowSize({350, 120});
	auto res = nc::theApplication().appConfiguration().resolution;
	ImGui::SetWindowPos({static_cast<float>(res.x)/2+175, 4*static_cast<float>(res.y)/5});
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
}

void MenuState::CleanUp() {
    logo.reset(nullptr);
    logoSprite.reset(nullptr);
}
