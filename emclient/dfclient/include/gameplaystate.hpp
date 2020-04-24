#pragma once

#include "gamestate.hpp"

class GameplayState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;
};