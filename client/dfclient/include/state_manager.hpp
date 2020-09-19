#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "game_state.hpp"
#include "resources.hpp"

struct StateManager {
	void AddState(StateType s, std::unique_ptr<GameState>&& state);
	void EnterState(StateType s);
	void OnFrameStart();
	void OnInit();
	void OnKeyPressed(const ncine::KeyboardEvent &event);
	void OnKeyReleased(const ncine::KeyboardEvent &event);
	void OnMouseButtonPressed(const ncine::MouseEvent &event);
	void OnMouseMoved(const ncine::MouseState &state);

	std::map<StateType, std::unique_ptr<GameState>> states;

	GameState* currentState = nullptr;
	std::optional<StateType> currentStateType = std::nullopt;

	Resources _resources;
};
