#include <iostream>
#include <functional>

#include <ncine/Application.h>
#include <ncine/Colorf.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>

#include "gameplay_state.hpp"
#include "game_data.hpp"
#include "fb_util.hpp"
#include "state_manager.hpp"

void GameplayState::LoadLevel() {
    auto level = FBUtilGetServerEvent(gameData.levelData, Level);

    for (int i = 0; i < level->stones()->size(); i++) {
        auto stone = level->stones()->Get(i);
        auto stoneSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), manager.textures["stone.png"].get(),
            stone->pos()->x() * METERS2PIXELS, -stone->pos()->y() * METERS2PIXELS);
        stoneSprite->setLayer(23);
        this->nodes.push_back(std::move(stoneSprite));
    }

    for (int i = 0; i < level->bushes()->size(); i++) {
        auto bush = level->bushes()->Get(i);
        auto bushSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), manager.textures["bush.png"].get(),
            bush->pos()->x() * METERS2PIXELS, -bush->pos()->y() * METERS2PIXELS);
        bushSprite->setLayer(24);
        this->nodes.push_back(std::move(bushSprite));
    }
}

void GameplayState::OnMessage(const std::string& data) {

}

void GameplayState::Create() {
    std::cout << "entered gameplay state\n";
    ncine::theApplication().gfxDevice().setClearColor(ncine::Colorf(1, 1, 1, 1));
    auto& rootNode = ncine::theApplication().rootNode();
    this->cameraNode = std::make_unique<ncine::SceneNode>(&rootNode);
    this->LoadLevel();
    gameData.socket->onMessage = std::bind(&GameplayState::OnMessage, this, std::placeholders::_1);
}

void GameplayState::Update() {

}

void GameplayState::CleanUp() {

}

void GameplayState::OnMouseMoved(const ncine::MouseState &state) {
    
}