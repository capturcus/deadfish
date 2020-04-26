#pragma once

#include "game_state.hpp"

struct MenuState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;

    bool TryConnect();
};