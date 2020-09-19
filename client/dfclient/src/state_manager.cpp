#include <iostream>

#include <ncine/FileSystem.h>
#include <ncine/Texture.h>
#include <ncine/Application.h>

#include "menu_state.hpp"
#include "lobby_state.hpp"
#include "gameplay_state.hpp"

#include "state_manager.hpp"
#include "websocket.hpp"

const char* TEXTURES_PATH = "textures";

StateManager::StateManager() {
	auto rootPath = ncine::theApplication().appConfiguration().dataPath();
	auto dir = ncine::FileSystem::Directory((rootPath + TEXTURES_PATH).data());
	const char* file = dir.readNext();
	while (file)
	{
		auto absPath = rootPath.data() + std::string(TEXTURES_PATH) + "/" + std::string(file);
		if (ncine::FileSystem::isFile(absPath.c_str())) {
			std::cout << "loading " << file << " " << absPath << "\n";
			_resources.textures[file] = std::make_unique<ncine::Texture>(absPath.c_str());
		}
		file = dir.readNext();
	}
	_resources.fonts["comic"] = std::make_unique<ncine::Font>((rootPath + "fonts/comic.fnt").data(), (rootPath + "fonts/comic.png").data());

	_resources._wilhelmAudioBuffer = std::make_unique<ncine::AudioBuffer>((rootPath + "/sounds/wilhelm.wav").data());
	_resources._wilhelmSound = std::make_unique<ncine::AudioBufferPlayer>(_resources._wilhelmAudioBuffer.get());

	_currentStateType = StateType::Menu;
	_currentState = std::make_unique<MenuState>(_resources);
}

void StateManager::OnFrameStart() {
	auto nextStateType = _currentState->Update(webSocketManager.GetMessages());
	if (nextStateType == _currentStateType) {
		return;
	}

	_currentStateType = nextStateType;
	switch (nextStateType) {
		case StateType::Menu:
			_currentState = std::make_unique<MenuState>(_resources);
			break;
		case StateType::Lobby:
			_currentState = std::make_unique<LobbyState>(_resources);
			break;
		case StateType::Gameplay:
			_currentState = std::make_unique<GameplayState>(_resources);
			break;
	}
}

void StateManager::OnKeyPressed(const ncine::KeyboardEvent &event) {
	_currentState->OnKeyPressed(event);
}

void StateManager::OnKeyReleased(const ncine::KeyboardEvent &event) {
	_currentState->OnKeyReleased(event);
}

void StateManager::OnMouseButtonPressed(const ncine::MouseEvent &event) {
	_currentState->OnMouseButtonPressed(event);
}

void StateManager::OnMouseMoved(const ncine::MouseState &state) {
	_currentState->OnMouseMoved(state);
}
