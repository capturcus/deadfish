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
#include "state_manager.hpp"
#include "menu_state.hpp"
#include "lobby_state.hpp"
#include "gameplay_state.hpp"
#include "game_data.hpp"

nc::IAppEventHandler *createAppEventHandler()
{
	return new MyEventHandler;
}

void MyEventHandler::onPreInit(nc::AppConfiguration &config)
{
	// config.withDebugOverlay = true;
	// config.resolution.x = 1800;
	// config.resolution.y = 1000;
#if defined(__EMSCRIPTEN__)
	config.dataPath() = "/";
#else
	config.dataPath() = "data/";
#endif
}

GameData gameData;
std::optional<StateManager> stateManager;
std::optional<TextCreator> textCreator;

void MyEventHandler::onInit()
{
	auto& device = ncine::theApplication().gfxDevice();
	int bestModeNum = -1;
	uint64_t bestPixels = 0;
	for (int i = 0; i < device.numVideoModes(); i++) {
		auto& mode = device.videoMode(i);
		if (mode.width * mode.height > bestPixels) {
			bestPixels = mode.width * mode.height;
			bestModeNum = i;
		}
	}
	auto& bestMode = device.videoMode(bestModeNum);
	std::cout << "best mode " << bestModeNum << " " <<
		bestMode.width << "x" << bestMode.height << "\n";
	device.setResolution({ bestMode.width, bestMode.height });
	device.setFullScreen(true);

	stateManager.emplace();
	textCreator.emplace(stateManager->CreateTextCreator());
}

void MyEventHandler::onFrameStart()
{
	stateManager->OnFrameStart();
}

void MyEventHandler::onKeyPressed(const nc::KeyboardEvent &event)
{
	stateManager->OnKeyPressed(event);
}

void MyEventHandler::onKeyReleased(const nc::KeyboardEvent &event)
{
	stateManager->OnKeyReleased(event);
}

void MyEventHandler::onMouseButtonPressed(const nc::MouseEvent &event)
{
	stateManager->OnMouseButtonPressed(event);
}

void MyEventHandler::onMouseMoved(const nc::MouseState &state)
{
	stateManager->OnMouseMoved(state);
}
