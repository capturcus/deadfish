#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>

#include <ncine/TextNode.h>

#include "game_state.hpp"

using StateMap = std::map<std::string, std::unique_ptr<GameState>>;

struct StateManager {
	void AddState(std::string name, std::unique_ptr<GameState>&& state);
	void EnterState(std::string name);
	void OnFrameStart();
	void OnInit();
	void OnKeyPressed(const ncine::KeyboardEvent &event);
	void OnKeyReleased(const ncine::KeyboardEvent &event);
	void OnMouseButtonPressed(const ncine::MouseEvent &event);
	void OnMouseMoved(const ncine::MouseState &state);

	StateMap states;
	GameState* currentState = nullptr;
	std::map<std::string, std::unique_ptr<ncine::Font>> fonts;
	std::map<std::string, std::unique_ptr<ncine::Texture>> textures;
};
