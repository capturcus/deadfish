#include <iostream>

#include <ncine/FileSystem.h>
#include <ncine/Texture.h>
#include <ncine/Application.h>

#include "state_manager.hpp"
#include "websocket.hpp"

void StateManager::AddState(StateType s, std::unique_ptr<GameState>&& state) {
	this->states[s] = std::move(state);
}

void StateManager::EnterState(StateType s) {
	if (currentState)
		currentState->CleanUp();
	currentState = states[s].get();
	currentStateType = s;
	currentState->Create();
}

const char* TEXTURES_PATH = "textures";

void StateManager::OnInit() {
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
}

void StateManager::OnFrameStart() {
	auto nextStateType = currentState->Update(webSocketManager.GetMessages());
	if (nextStateType != currentStateType) {
		EnterState(nextStateType);
	}
}

void StateManager::OnKeyPressed(const ncine::KeyboardEvent &event) {
	currentState->OnKeyPressed(event);
}

void StateManager::OnKeyReleased(const ncine::KeyboardEvent &event) {
	currentState->OnKeyReleased(event);
}

void StateManager::OnMouseButtonPressed(const ncine::MouseEvent &event) {
	currentState->OnMouseButtonPressed(event);
}

void StateManager::OnMouseMoved(const ncine::MouseState &state) {
	currentState->OnMouseMoved(state);
}
