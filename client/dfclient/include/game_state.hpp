#pragma once

#include <ncine/IInputEventHandler.h>

enum class StateType {
	Menu,
	Lobby,
	Gameplay
};

struct GameState {
    virtual void Create() = 0;
    virtual StateType Update() = 0;
    virtual void CleanUp() = 0;
    virtual void OnKeyPressed(const ncine::KeyboardEvent &event) {}
	virtual void OnKeyReleased(const ncine::KeyboardEvent &event) {}
	virtual void OnMouseButtonPressed(const ncine::MouseEvent &event) {}
	virtual void OnMouseMoved(const ncine::MouseState &state) {}
    virtual ~GameState() {}
};
