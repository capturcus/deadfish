#pragma once

#include "game_state.hpp"

class Resources;

struct MenuState
	: public GameState
{
public:
	MenuState(Resources& r) : _resources(r) {}
	
	void Create() override;
	StateType Update(Messages) override;
	void CleanUp() override;

	bool TryConnect();

private:
	Resources& _resources;
};
