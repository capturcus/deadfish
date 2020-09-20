#pragma once

#include <memory>
#include <vector>

#include <ncine/TextNode.h>

#include "game_state.hpp"
#include "websocket.hpp"

class Resources;

struct LobbyState
	: public GameState
{
	LobbyState(Resources& r);
	~LobbyState() override;

	StateType Update(Messages m) override;
	void OnMouseButtonPressed(const ncine::MouseEvent &event) override;

private:
	StateType OnMessage(const std::string& data);

	void RedrawPlayers();

	void SendPlayerReady();

	std::vector<std::unique_ptr<ncine::SceneNode>> sceneNodes;
	std::vector<std::unique_ptr<ncine::TextNode>> textNodes;
	std::unique_ptr<ncine::TextNode> readyButton;
	bool ready = false;

	Resources& _resources;
};
