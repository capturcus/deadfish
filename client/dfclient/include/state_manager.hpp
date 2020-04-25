#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>

#include <ncine/TextNode.h>

#include "game_state.hpp"

using StateMap = std::map<std::string, std::unique_ptr<GameState>>;

struct StateManager {
    void AddState(std::string name, std::unique_ptr<GameState>&& state);
    void EnterState(std::string name);
    void OnFrameStart();
    void OnInit();

    StateMap states;
    GameState* currentState = nullptr;
    std::map<std::string, std::unique_ptr<ncine::Font>> fonts;
};