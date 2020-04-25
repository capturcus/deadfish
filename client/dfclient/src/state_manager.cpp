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

void StateManager::OnInit() {
    fonts["comic"] = std::make_unique<ncine::Font>("fonts/comic.fnt", "fonts/comic.png");
}

void StateManager::OnFrameStart() {
    currentState->Update();
}