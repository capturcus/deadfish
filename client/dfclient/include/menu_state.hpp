#pragma once

#include "game_state.hpp"

#include "tweeny.h"

struct MenuState
	: public GameState
{
	using GameState::GameState;
	
	void Create() override;
	void Update() override;
	void CleanUp() override;

	bool TryConnect();
};
