#pragma once

#include <vector>
#include <memory>
#include <map>

#include <ncine/Sprite.h>

#include "game_state.hpp"
#include "../../../common/deadfish_generated.h"

const float PIXELS2METERS = 0.01f;
const float METERS2PIXELS = 100.f;

struct Mob {
    std::unique_ptr<ncine::AnimatedSprite> sprite;
    bool seen;
    DeadFish::MobState state = DeadFish::MobState_Walk;
};

struct GameplayState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;

    void OnMessage(const std::string& data);
    void OnMouseButtonPressed(const ncine::MouseEvent &event) override;
    void OnKeyPressed(const ncine::KeyboardEvent &event) override;
    void OnKeyReleased(const ncine::KeyboardEvent &event) override;
    void LoadLevel();
    std::unique_ptr<ncine::AnimatedSprite> CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species);

    std::vector<std::unique_ptr<ncine::DrawableNode>> nodes;
    std::unique_ptr<ncine::SceneNode> cameraNode;
    std::map<uint16_t, Mob> mobs;
    ncine::Sprite* mySprite = nullptr;
};