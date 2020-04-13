#pragma once

#include "gamestate.hpp"

class LobbyState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
};