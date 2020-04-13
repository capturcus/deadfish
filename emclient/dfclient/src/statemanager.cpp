#include "statemanager.hpp"

void StateManager::AddState(std::string name, std::unique_ptr<GameState>&& state) {
    this->states[name] = std::move(state);
}

void StateManager::EnterState(std::string name) {
    // cleanup the previous state
    currentState = states[name].get();
    // init the next state
}