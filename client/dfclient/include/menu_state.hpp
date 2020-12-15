#pragma once

#include "game_state.hpp"

#include "tweeny.h"

class Resources;

struct MenuState
	: public GameState
{
public:
	MenuState(Resources& r);
	~MenuState() override;
	
	StateType Update(Messages) override;

private:
	bool TryConnect();
	void ProcessMatchmakerData(std::string data);
	void ShowMessage(std::string message);

	Resources& _resources;
};
