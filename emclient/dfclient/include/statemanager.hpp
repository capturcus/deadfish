#pragma once
#include <string>
#include <memory>
#include <map>
#include "gamestate.hpp"

using StateMap = std::map<std::string, std::unique_ptr<GameState>>;

class StateManager {
public:
    void AddState(std::string name, std::unique_ptr<GameState>&& state);
    void EnterState(std::string name);
    void OnInit();
    void OnFrameStart();

    StateMap states;
    GameState* currentState;
};