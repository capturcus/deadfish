#include <iostream>
#include <functional>

#include <ncine/Application.h>
#include <ncine/Colorf.h>

#include "gameplay_state.hpp"
#include "game_data.hpp"
#include "fb_util.hpp"

void GameplayState::LoadLevel() {
    // auto level = FBUtilGetServerEvent(gameData.levelData, Level);
    // for (int i = 0; i < level->bushes()->size(); i++) {
    //     auto bush = level->bushes()->Get(i);
        
    // }
}

void GameplayState::OnMessage(const std::string& data) {

}

void GameplayState::Create() {
    std::cout << "entered gameplay state\n";
    ncine::theApplication().gfxDevice().setClearColor(ncine::Colorf(1, 1, 1, 1));
    this->LoadLevel();
    gameData.socket->onMessage = std::bind(&GameplayState::OnMessage, this, std::placeholders::_1);
}

void GameplayState::Update() {

}

void GameplayState::CleanUp() {

}
