#include <iostream>
#include <utility>
#include "flatbuffers/flatbuffers.h"

#include "agones.hpp"
#include "deadfish.hpp"
#include "game_thread.hpp"
#include "websocket.hpp"

GameState gameState;

void sendInitMetadata()
{
	for (auto &targetPlayerIt : gameState.players)
	{
		auto &targetPlayer = targetPlayerIt.second;
		flatbuffers::FlatBufferBuilder builder(1);
		std::vector<flatbuffers::Offset<FlatBuffGenerated::InitPlayer>> playerOffsets;
		for (auto& playerIt : gameState.players)
		{
			auto& player = playerIt.second;
			auto name = builder.CreateString(player->name.c_str());
			auto playerOffset = FlatBuffGenerated::CreateInitPlayer(builder, player->playerID, name, player->species, player->ready);
			playerOffsets.push_back(playerOffset);
		}
		auto players = builder.CreateVector(playerOffsets);
		auto metadata = FlatBuffGenerated::CreateInitMetadata(builder, players, targetPlayer->movableID, targetPlayer->playerID);
		sendServerMessage(*targetPlayer, builder, FlatBuffGenerated::ServerMessageUnion_InitMetadata, metadata.Union());
	}
}

void addNewPlayer(dfws::Handle hdl, const std::string &name)
{
	if (gameState.phase != GamePhase::LOBBY)
	{
		return;
	}

	// TODO: check that a player with the same name is not present
	auto p = std::make_unique<Player>();
	p->movableID = newMovableID();
	p->name = name;
	p->wsHandle = hdl;
	p->playerID = gameState.players.size();

	gameState.players[p->movableID] = std::move(p);

	agones::SetPlayers(gameState.players.size());
	sendInitMetadata();
}

void startGame() {
	gameState.phase = GamePhase::GAME;
	dfws::SetOnMessage(&gameOnMessage);
	agones::SetPlaying();
	new std::thread(gameThread); // leak the shit out of it yooo
}

void mainOnMessage(dfws::Handle hdl, const std::string& payload)
{
	if (payload.size() == 0)
		return; // wtf

	const auto clientMessage = flatbuffers::GetRoot<FlatBuffGenerated::ClientMessage>(payload.c_str());

	switch (clientMessage->event_type())
	{
	case FlatBuffGenerated::ClientMessageUnion::ClientMessageUnion_JoinRequest:
	{
		const auto event = clientMessage->event_as_JoinRequest();
		if (gameState.players.size() == 6) {
			sendGameAlreadyInProgress(hdl);
			std::cout << "player " << event->name()->c_str() << " dropped, too many players\n";
			return;
		}
		std::cout << "new player " << event->name()->c_str() << "\n";
		addNewPlayer(hdl, event->name()->c_str());
		std::cout << "player count " << gameState.players.size() << "\n";
		if (gameState.options.count("numplayers")) {
			auto numplayers = gameState.options["numplayers"].as<unsigned long>();
			if (numplayers == gameState.players.size())
				startGame();
		}
	}
	break;
	case FlatBuffGenerated::ClientMessageUnion::ClientMessageUnion_PlayerReady:
	{
		auto pl = getPlayerByConnHdl(hdl);
		if (!pl) {
			sendGameAlreadyInProgress(hdl);
			return;
		}
		pl->ready = true;
		sendInitMetadata();
		if (gameState.options.count("numplayers"))
			return; // the game will start after a number of player will join, not after all being ready
		for (auto& p : gameState.players) {
			if (!p.second->ready)
				return;
		}
		startGame();
	}
	break;

	default:
		std::cout << "on_message: other message type received\n";
		break;
	}
}

void mainOnClose(dfws::Handle hdl)
{
	auto playerIt = gameState.players.begin();
	while (playerIt != gameState.players.end())
	{
		auto &player = playerIt->second;
		if (player->wsHandle == hdl)
		{
			std::cout << "deleting player " << player->name << "\n";
			break;
		}
		playerIt++;
	}
	if (playerIt != gameState.players.end())
		gameState.players.erase(playerIt);

	if (gameState.phase == GamePhase::LOBBY)
	{
		agones::SetPlayers(gameState.players.size());
		sendInitMetadata();
	}
	if (gameState.phase == GamePhase::GAME && gameState.players.empty())
	{
		std::cout << "no players left, exiting\n";
		agones::Shutdown();
	}
}

void mainOnOpen(dfws::Handle hdl) {
	if (gameState.phase == GamePhase::GAME) {
		sendGameAlreadyInProgress(hdl);
		return;
	}
}

template<typename T>
bool ensureMandatoryOption(const char* opt) {
	if (gameState.options.count(opt)) {
		std::cout << opt << " = " << gameState.options[opt].as<T>() << "\n";
	} else {
		std::cout << opt << " option is required to start\n";
		return false;
	}
	return true;
}

// returns true if we can proceed
bool handleCliOptions(int argc, const char* const argv[]) {
	boost_po::options_description desc("Deadfish server options");
	desc.add_options()
		("help,h", "show help message")
		("test,t", "enter test mode")
		("port,p", boost_po::value<int>(), "the port on which the server will be accepting connections")
		("level,l", boost_po::value<std::string>(), "level flatbuffer file to be loaded by the server")
		("numplayers,n", boost_po::value<unsigned long>(), "the server will launch the game after the specified amount of players will appear in lobby, not when everybody is ready")
		("ghosttown,g", boost_po::value<bool>()->default_value(false)->implicit_value(true), "no mobs mode" )
		("agones", boost_po::value<bool>()->default_value(false)->implicit_value(true), "run the server with agones sdk thread" )
	;

	boost_po::store(boost_po::parse_command_line(argc, argv, desc), gameState.options);
	boost_po::notify(gameState.options);    

	if (gameState.options.count("help")) {
		std::cout << desc << "\n";
		return false;
	}

	if (!ensureMandatoryOption<int>("port") || !ensureMandatoryOption<std::string>("level"))
		return false;

	return true;
}

int main(int argc, const char* const argv[])
{
	srand(time(nullptr));

	if (!handleCliOptions(argc, argv))
		return 1;

	dfws::SetOnMessage(&mainOnMessage);
	dfws::SetOnOpen(&mainOnOpen);
	dfws::SetOnClose(&mainOnClose);

	std::cout << "server started\n";

	if (gameState.options["agones"].as<bool>())
		if (!agones::Start()) {
			std::cout << "failed to initalize agones\n";
			return -1;
		}

	int port = gameState.options["port"].as<int>();

	dfws::Run(port);

	std::cout << "server stopped\n";
	return 0;
}
