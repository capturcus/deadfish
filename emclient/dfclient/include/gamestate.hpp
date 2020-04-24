#pragma once

class StateManager;

class GameState {
public:
    GameState(StateManager& manager_)
        : manager(manager_) {}
    virtual void Create() = 0;
    virtual void Update() = 0;
    virtual void CleanUp() = 0;
    virtual ~GameState() {}

    StateManager& manager;
};