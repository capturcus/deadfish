#include <iostream>

#include <ncine/Application.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>
#include <ncine/imgui.h>

#include "menustate.hpp"
#include "statemanager.hpp"

namespace nc = ncine;

nctl::UniquePtr<nc::Texture> logo;
nctl::UniquePtr<nc::Sprite> logoSprite;
nc::Colorf bgColor(0.96875f, 0.97265625, 0.953125, 1.0f);

void MenuState::Create() {
    std::cout << "menu create\n";
    nc::SceneNode &rootNode = nc::theApplication().rootNode();
	// make sprites and stuff
	logo = nctl::makeUnique<nc::Texture>("/deadfish.png");
	auto res = nc::theApplication().appConfiguration().resolution;
	logoSprite = nctl::makeUnique<nc::Sprite>(&rootNode, logo.get(), res.x, res.y);
	logoSprite->setPosition(logoSprite->position() + ncine::Vector2f{-logoSprite->width()/2, 0});

	nc::theApplication().gfxDevice().setClearColor(bgColor);
}

void MenuState::Update() {
    std::cout << "menu update\n";
    ImGui::Begin("DeadFish", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowSize({350, 120});
	auto res = nc::theApplication().appConfiguration().resolution;
	ImGui::SetWindowPos({static_cast<float>(res.x)/2+175, 4*static_cast<float>(res.y)/5});
	static char buf1[64] = ""; ImGui::InputText("server", buf1, 64);
	static char buf2[64] = ""; ImGui::InputText("nickname", buf2, 64);
	if (ImGui::Button("connect", {300, 30})) {
        this->manager.EnterState("lobby");
    }
	ImGui::End();
}

void MenuState::CleanUp() {
    logo.reset(nullptr);
    logoSprite.reset(nullptr);
}
