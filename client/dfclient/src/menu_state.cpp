#include <iostream>
#include <functional>

#include <ncine/Application.h>
#include <ncine/imgui.h>
#include <ncine/MeshSprite.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>

#include "menu_state.hpp"
#include "state_manager.hpp"
#include "game_data.hpp"

namespace nc = ncine;

nctl::UniquePtr<nc::Sprite> logoSprite;
nc::Colorf bgColor(0.96875f, 0.97265625, 0.953125, 1.0f);
std::unique_ptr<nc::MeshSprite> testMesh;

inline nc::Vector2f rotateVector(const nc::Vector2f& v, float angle) {
	return nc::Vector2f(v.x*cos(angle) + v.y*(-sin(angle)), v.x*sin(angle) + v.y*cos(angle));
}

std::vector<nc::Vector2f> createRingTexels(float outerRadius, float innerRadius)
{
	std::vector<nc::Vector2f> ret;
	// start from left
	nc::Vector2f inner = {-innerRadius, 0};
	nc::Vector2f outer = {-outerRadius, 0};
	for (int i = 0; i < 17; i++) {
		ret.push_back(outer);
		ret.push_back(inner);
		outer = rotateVector(outer, M_PI / 8);
		inner = rotateVector(inner, M_PI / 8);
	}
	return ret;
}

void MenuState::Create() {
    nc::SceneNode &rootNode = nc::theApplication().rootNode();
	auto res = nc::theApplication().appConfiguration().resolution;
	std::cout << "res " << res.x << "," << res.y << "\n";
	logoSprite = nctl::makeUnique<nc::Sprite>(&rootNode, manager.textures["deadfish.png"].get(), res.x*0.5f, res.y*0.6f);
	// logoSprite->setPosition(logoSprite->position() + ncine::Vector2f{-logoSprite->width()/2, 0});

	nc::theApplication().gfxDevice().setClearColor(bgColor);

	testMesh = std::make_unique<nc::MeshSprite>(&rootNode, manager.textures["square.png"].get(), 500, 500);
	auto texels = createRingTexels(150, 100);
	std::cout << "printing ring texels " << texels.size() << "\n";
	for (auto& t: texels) {
		std::cout << t.x << "," << t.y << "\n";
	}
	testMesh->createVerticesFromTexels(texels.size(), texels.data());
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
}

void MenuState::CleanUp() {
    logoSprite.reset(nullptr);
}
