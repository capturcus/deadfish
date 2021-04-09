#include <iostream>

#include "resources.hpp"
#include "loading_state.hpp"
#include "menu_state.hpp"
#include "lobby_state.hpp"
#include "gameplay_state.hpp"

#include "state_manager.hpp"
#include "websocket.hpp"

StateManager::StateManager() {
	// init LoadingState
	_currentStateType = StateType::Loading;
	_currentState = std::make_unique<LoadingState>(_resources);
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
