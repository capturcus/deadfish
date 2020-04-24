#include "statemanager.hpp"

void StateManager::AddState(std::string name, std::unique_ptr<GameState>&& state) {
    this->states[name] = std::move(state);
}

void StateManager::EnterState(std::string name) {
    if (currentState)
        currentState->CleanUp();
    currentState = states[name].get();
    currentState->Create();
}

void StateManager::OnFrameStart() {
    currentState->Update();
}