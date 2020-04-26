#pragma once

#include <vector>
#include <memory>

#include <ncine/Sprite.h>

#include "game_state.hpp"

const float PIXELS2METERS = 0.01f;
const float METERS2PIXELS = 100.f;

struct GameplayState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;

    void OnMessage(const std::string& data);
    void OnMouseMoved(const ncine::MouseState &state) override;
    void LoadLevel();

    std::vector<std::unique_ptr<ncine::DrawableNode>> nodes;
    std::unique_ptr<ncine::SceneNode> cameraNode;
};