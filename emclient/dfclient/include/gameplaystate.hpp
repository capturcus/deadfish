#pragma once

#include "gamestate.hpp"

struct GameplayState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;
};