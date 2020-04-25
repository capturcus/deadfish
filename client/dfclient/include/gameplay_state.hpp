#pragma once

#include "game_state.hpp"

struct GameplayState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;
};