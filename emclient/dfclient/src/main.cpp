#include <iostream>
#include <memory>
#include <ncine/common_constants.h>
#include <ncine/Application.h>
#include <ncine/Texture.h>
#include <ncine/AnimatedSprite.h>
#include <ncine/AppConfiguration.h>
#include <ncine/FileSystem.h>
#include <ncine/imgui.h>

#include "main.h"
#include "statemanager.hpp"
#include "menustate.hpp"
#include "lobbystate.hpp"

nc::IAppEventHandler *createAppEventHandler()
{
	return new MyEventHandler;
}

void MyEventHandler::onPreInit(nc::AppConfiguration &config)
{
	// config.withDebugOverlay = true;
	config.dataPath() = "/";
}

// nctl::UniquePtr<nc::Texture> logo;
// nctl::UniquePtr<nc::Sprite> logoSprite;
// nc::Colorf bgColor(0.96875f, 0.97265625, 0.953125, 1.0f);

StateManager manager;

void MyEventHandler::onInit()
{
	// nc::SceneNode &rootNode = nc::theApplication().rootNode();
	// // make sprites and stuff
	// logo = nctl::makeUnique<nc::Texture>("/deadfish.png");
	// auto res = nc::theApplication().appConfiguration().resolution;
	// logoSprite = nctl::makeUnique<nc::Sprite>(&rootNode, logo.get(), res.x, res.y);
	// logoSprite->setPosition(logoSprite->position() + ncine::Vector2f{-logoSprite->width()/2, 0});
	manager.AddState("menu", std::make_unique<MenuState>(manager));
	manager.AddState("lobby", std::make_unique<LobbyState>(manager));
	manager.EnterState("menu");
}

void MyEventHandler::onFrameStart()
{
	// ImGui::Begin("DeadFish", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
	// 	ImGuiWindowFlags_NoCollapse);
	// ImGui::SetWindowSize({350, 120});
	// auto res = nc::theApplication().appConfiguration().resolution;
	// ImGui::SetWindowPos({static_cast<float>(res.x)/2+175, 4*static_cast<float>(res.y)/5});
	// static char buf1[64] = ""; ImGui::InputText("server", buf1, 64);
	// static char buf2[64] = ""; ImGui::InputText("nickname", buf2, 64);
	// ImGui::Button("connect", {300, 30});
	// ImGui::End();

	// nc::theApplication().gfxDevice().setClearColor(bgColor);
}

void MyEventHandler::onKeyReleased(const nc::KeyboardEvent &event)
{
}

void MyEventHandler::onMouseButtonPressed(const nc::MouseEvent &event)
{
}

void MyEventHandler::onMouseMoved(const nc::MouseState &state)
{
}