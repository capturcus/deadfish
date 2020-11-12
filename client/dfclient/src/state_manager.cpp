#include <iostream>

#include <ncine/FileSystem.h>
#include <ncine/Texture.h>
#include <ncine/Application.h>

#include "resources.hpp"
#include "menu_state.hpp"
#include "lobby_state.hpp"
#include "gameplay_state.hpp"

#include "state_manager.hpp"
#include "websocket.hpp"

StateManager::StateManager() {
	// load textures
	auto rootPath = ncine::theApplication().appConfiguration().dataPath();
	auto textureDir = ncine::FileSystem::Directory((rootPath + TEXTURES_PATH).data());
	const char* file = textureDir.readNext();
	while (file)
	{
		auto absPath = rootPath.data() + std::string(TEXTURES_PATH) + "/" + std::string(file);
		if (ncine::FileSystem::isFile(absPath.c_str())) {
			std::cout << "loading " << file << " " << absPath << "\n";
			_resources.textures[file] = std::make_unique<ncine::Texture>(absPath.c_str());
		}
		file = textureDir.readNext();
	}
	textureDir.close();
	_resources.fonts["comic"] = std::make_unique<ncine::Font>((rootPath + "fonts/comic.fnt").data(), (rootPath + "fonts/comic.png").data());

	// load sounds
	auto soundDir = ncine::FileSystem::Directory((rootPath + SOUNDS_PATH).data());
	file = soundDir.readNext();
	while (file) {
		auto absPath = rootPath.data() + std::string(SOUNDS_PATH) + "/" + std::string(file);
		if (ncine::FileSystem::isFile(absPath.c_str())) {
			std::cout << "loading sound " << file << " " << absPath << "\n";
			_resources._sounds[file] = std::make_unique<ncine::AudioBuffer>(absPath.c_str());
		}
		file = soundDir.readNext();
	}

	_resources._sounds.erase("you-failed.wav"); // temporary disable, it will be useful as a gameover sound effect

	_resources._killSoundBuffer = std::move(_resources._sounds["amongus-kill.wav"]);
	_resources._sounds.erase("amongus-kill.wav");
	_resources._killSound = std::make_unique<ncine::AudioBufferPlayer>(_resources._killSoundBuffer.get());

	_resources._goldfishSoundBuffer = std::move(_resources._sounds["mario-powerup.wav"]);
	_resources._sounds.erase("mario-powerup.wav");
	_resources._goldfishSound = std::make_unique<ncine::AudioBufferPlayer>(_resources._goldfishSoundBuffer.get());
	

	_currentStateType = StateType::Menu;
	_currentState = std::make_unique<MenuState>(_resources);
}

TextCreator StateManager::CreateTextCreator() {
	return TextCreator(_resources);
}

void StateManager::OnFrameStart() {
	_resources.UpdateTweens();

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
