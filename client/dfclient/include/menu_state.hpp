#pragma once

#include "game_state.hpp"

class Resources;

struct MenuState
	: public GameState
{
public:
	MenuState(Resources& r) : _resources(r) {}
	
	void Create() override;
	StateType Update() override;
	void CleanUp() override;

	bool TryConnect();

private:
	bool enterLobbyOnNextUpdate = false;
	Resources& _resources;
};
