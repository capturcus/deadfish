#pragma once

#include "game_state.hpp"
#include "ncine/TextNode.h"

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

	std::unique_ptr<ncine::TextNode> gameInProgressText;

	Resources& _resources;
};
