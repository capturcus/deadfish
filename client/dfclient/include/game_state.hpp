#pragma once

#include <ncine/IInputEventHandler.h>

#include "messages.hpp"

enum class StateType {
	Loading,
	Menu,
	Lobby,
	Gameplay
};

struct GameState {
	virtual StateType Update(Messages) = 0;
	virtual void OnKeyPressed(const ncine::KeyboardEvent &event) {}
	virtual void OnKeyReleased(const ncine::KeyboardEvent &event) {}
	virtual void OnMouseButtonPressed(const ncine::MouseEvent &event) {}
	virtual void OnMouseMoved(const ncine::MouseState &state) {}
	virtual ~GameState() {}
};
