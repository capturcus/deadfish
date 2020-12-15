#pragma once

#include "game_state.hpp"

#include "tweeny.h"

class Resources;

enum class ClientMode {
	Matchmaker,
	DirectConnect
};
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
	void MatchmakerLayout(char* buf);
	void DirectConnectLayout(char* buf0, char* buf1);

	ClientMode clientMode = ClientMode::Matchmaker;
	std::string matchmakerAddress = "localhost";
	std::string matchmakerPort = "8000";
	std::string directAddress = "localhost";
	std::string directPort = "63987";

	Resources& _resources;
};
