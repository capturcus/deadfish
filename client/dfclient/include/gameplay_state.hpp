#pragma once

#include <ncine/Sprite.h>

#include "game_state.hpp"

struct GameplayState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;

    void OnMessage(const std::string& data);
    void LoadLevel();

    std::vector<std::unique_ptr<ncine::DrawableNode>> nodes;
};