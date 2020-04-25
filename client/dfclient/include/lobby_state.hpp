#pragma once

#include <memory>
#include <vector>

#include <ncine/TextNode.h>

#include "game_state.hpp"
#include "websocket.hpp"


struct LobbyState
    : public GameState
{
    using GameState::GameState;
    
    void Create() override;
    void Update() override;
    void CleanUp() override;
    void RedrawPlayers();

    std::vector<std::unique_ptr<ncine::SceneNode>> sceneNodes;
    std::vector<std::unique_ptr<ncine::TextNode>> textNodes;
    std::unique_ptr<ncine::TextNode> readyButton;
    bool ready = false;
};