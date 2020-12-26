#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "tweeny.h"

#include "game_state.hpp"
#include "resources.hpp"
#include "text_creator.hpp"

struct StateManager {
	StateManager();

	void OnFrameStart();
	void OnKeyPressed(const ncine::KeyboardEvent &event);
	void OnKeyReleased(const ncine::KeyboardEvent &event);
	void OnMouseButtonPressed(const ncine::MouseEvent &event);
	void OnMouseMoved(const ncine::MouseState &state);

	TextCreator CreateTextCreator();

private:
	std::unique_ptr<GameState> _currentState;
	StateType _currentStateType;

	Resources _resources;
};
