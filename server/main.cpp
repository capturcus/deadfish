#include <iostream>
#include <utility>
#include "flatbuffers/flatbuffers.h"

#include "deadfish.hpp"
#include "game_thread.hpp"

GameState gameState;
server websocket_server;

void sendInitMetadata()
{
	for (auto &targetPlayer : gameState.players)
	{
		flatbuffers::FlatBufferBuilder builder(1);
		std::vector<flatbuffers::Offset<FlatBuffGenerated::InitPlayer>> playerOffsets;
		for (size_t i = 0; i < gameState.players.size(); i++)
		{
			auto& player = gameState.players[i];
			auto name = builder.CreateString(player->name.c_str());
			auto playerOffset = FlatBuffGenerated::CreateInitPlayer(builder, i, name, player->species, player->ready);
			playerOffsets.push_back(playerOffset);
		}
		auto players = builder.CreateVector(playerOffsets);
		auto metadata = FlatBuffGenerated::CreateInitMetadata(builder, players, targetPlayer->mobID, targetPlayer->playerID);
		sendServerMessage(*targetPlayer, builder, FlatBuffGenerated::ServerMessageUnion_InitMetadata, metadata.Union());
	}
}

void addNewPlayer(const std::string &name, websocketpp::connection_hdl hdl)
{
	if (gameState.phase != GamePhase::LOBBY)
	{
		return;
	}

	// TODO: check that a player with the same name is not present
	auto p = std::make_unique<Player>();
	p->mobID = newMobID();
	p->name = name;
	p->conn_hdl = std::move(hdl);
	p->playerID = gameState.players.size();

	gameState.players.push_back(std::move(p));

	sendInitMetadata();
}

void startGame() {
	gameState.phase = GamePhase::GAME;
	for (auto &p : gameState.players)
	{
		auto con = websocket_server.get_con_from_hdl(p->conn_hdl);
		con->set_message_handler(&gameOnMessage);
	}
	new std::thread(gameThread); // leak the shit out of it yooo
}

void on_message(websocketpp::connection_hdl hdl, const server::message_ptr& msg)
{
	std::cout << "on_message\n";
	std::cout << "message from " << (uint64_t)hdl.lock().get() << "\n";

	const auto payload = msg->get_payload();
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
		addNewPlayer(event->name()->c_str(), hdl);
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
			if (!p->ready)
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

void on_close(websocketpp::connection_hdl hdl)
{
	auto player = gameState.players.begin();
	while (player != gameState.players.end())
	{
		if ((*player)->conn_hdl == hdl)
		{
			std::cout << "deleting player " << (*player)->name << "\n";
			break;
		}
		player++;
	}
	if (player != gameState.players.end())
		gameState.players.erase(player);

	if (gameState.phase == GamePhase::LOBBY)
	{
		sendInitMetadata();
	}
	if (gameState.phase == GamePhase::GAME && gameState.players.empty())
	{
		std::cout << "no players left, exiting\n";
		exit(0);
	}
}

void on_open(websocketpp::connection_hdl hdl) {
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

	websocket_server.get_alog().clear_channels(websocketpp::log::alevel::all);

	websocket_server.set_open_handler(&on_open);
	websocket_server.set_message_handler(&on_message);
	websocket_server.set_reuse_addr(true);
	websocket_server.set_close_handler(&on_close);

	websocket_server.init_asio();
	int port = gameState.options["port"].as<int>();
	websocket_server.listen(port);
	websocket_server.start_accept();

	std::cout << "server started\n";

	websocket_server.run();

	std::cout << "server stopped\n";
	return 0;
}
