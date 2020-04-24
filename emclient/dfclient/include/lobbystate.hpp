#pragma once

#include <memory>

#include "gamestate.hpp"
#include "websocket.hpp"

struct LobbyState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;

    std::unique_ptr<WebSocket> socket = nullptr;
};