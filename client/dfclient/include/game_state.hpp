#pragma once

#include <ncine/IInputEventHandler.h>

class StateManager;

struct GameState {
    GameState(StateManager& manager_)
        : manager(manager_) {}
    virtual void Create() = 0;
    virtual void Update() = 0;
    virtual void CleanUp() = 0;
    virtual void OnKeyPressed(const ncine::KeyboardEvent &event) {}
	virtual void OnKeyReleased(const ncine::KeyboardEvent &event) {}
	virtual void OnMouseButtonPressed(const ncine::MouseEvent &event) {}
	virtual void OnMouseMoved(const ncine::MouseState &state) {}
    virtual ~GameState() {}

    StateManager& manager;
};