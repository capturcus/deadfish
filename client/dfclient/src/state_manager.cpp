#include <iostream>

#include <ncine/FileSystem.h>
#include <ncine/Texture.h>

#include "state_manager.hpp"

void StateManager::AddState(std::string name, std::unique_ptr<GameState>&& state) {
    this->states[name] = std::move(state);
}

void StateManager::EnterState(std::string name) {
    if (currentState)
        currentState->CleanUp();
    currentState = states[name].get();
    currentState->Create();
}

const char* TEXTURES_PATH = "/textures";

void StateManager::OnInit() {
    auto dir = ncine::FileSystem::Directory(TEXTURES_PATH);
    const char* file = dir.readNext();
    while (file)
    {
        auto absPath = std::string(TEXTURES_PATH) + "/" + std::string(file);
        if (ncine::FileSystem::isFile(absPath.c_str())) {
            std::cout << "loading " << file << " " << absPath << "\n";
            textures[file] = std::make_unique<ncine::Texture>(absPath.c_str());
        }
        file = dir.readNext();
    }
    fonts["comic"] = std::make_unique<ncine::Font>("fonts/comic.fnt", "fonts/comic.png");
}

void StateManager::OnFrameStart() {
    currentState->Update();
}

void StateManager::OnKeyPressed(const ncine::KeyboardEvent &event) {
    currentState->OnKeyPressed(event);
}

void StateManager::OnKeyReleased(const ncine::KeyboardEvent &event) {
    currentState->OnKeyReleased(event);
}

void StateManager::OnMouseButtonPressed(const ncine::MouseEvent &event) {
    currentState->OnMouseButtonPressed(event);
}

void StateManager::OnMouseMoved(const ncine::MouseState &state) {
    currentState->OnMouseMoved(state);
}