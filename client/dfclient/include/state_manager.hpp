#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "game_state.hpp"
#include "resources.hpp"

struct StateManager {
	StateManager();

	void OnFrameStart();
	void OnKeyPressed(const ncine::KeyboardEvent &event);
	void OnKeyReleased(const ncine::KeyboardEvent &event);
	void OnMouseButtonPressed(const ncine::MouseEvent &event);
	void OnMouseMoved(const ncine::MouseState &state);

private:
	std::unique_ptr<GameState> _currentState;
	StateType _currentStateType;

	Resources _resources;
};
