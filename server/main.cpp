#include <iostream>
#include "flatbuffers/flatbuffers.h"

#include "deadfish.hpp"
#include "game_thread.hpp"
#include "level_loader.hpp"

GameState gameState;
server websocket_server;

void sendInitMetadata()
{
	for (auto &targetPlayer : gameState.players)
	{
		flatbuffers::FlatBufferBuilder builder(1);
		std::vector<flatbuffers::Offset<DeadFish::InitPlayer>> playerOffsets;
		for (size_t i = 0; i < gameState.players.size(); i++)
		{
			auto& player = gameState.players[i];
			auto name = builder.CreateString(player->name.c_str());
			auto playerOffset = DeadFish::CreateInitPlayer(builder, i, name, player->species, player->ready);
			playerOffsets.push_back(playerOffset);
		}
		auto players = builder.CreateVector(playerOffsets);
		auto metadata = DeadFish::CreateInitMetadata(builder, players, targetPlayer->mobID, targetPlayer->playerID);
		sendServerMessage(*targetPlayer.get(), builder, DeadFish::ServerMessageUnion_InitMetadata, metadata.Union());
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
	p->conn_hdl = hdl;
	p->playerID = gameState.players.size();

	gameState.players.push_back(std::move(p));

	sendInitMetadata();
}

void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
	std::cout << "on_message\n";
	std::cout << "message from " << (uint64_t)hdl.lock().get() << "\n";

	const auto payload = msg->get_payload();
	const auto clientMessage = flatbuffers::GetRoot<DeadFish::ClientMessage>(payload.c_str());

	switch (clientMessage->event_type())
	{
	case DeadFish::ClientMessageUnion::ClientMessageUnion_JoinRequest:
	{
		const auto event = clientMessage->event_as_JoinRequest();
		std::cout << "new player " << event->name()->c_str() << "\n";
		addNewPlayer(event->name()->c_str(), hdl);
		std::cout << "player count " << gameState.players.size() << "\n";
	}
	break;
	case DeadFish::ClientMessageUnion::ClientMessageUnion_PlayerReady:
	{
		auto pl = getPlayerByConnHdl(hdl);
		if (!pl) {
			sendGameAlreadyInProgress(hdl);
			return;
		}
		pl->ready = true;
		sendInitMetadata();
		for (auto& p : gameState.players) {
			if (!p->ready)
				return;
		}
		// all are ready, start game
		gameState.phase = GamePhase::GAME;
		for (auto &p : gameState.players)
		{
			auto con = websocket_server.get_con_from_hdl(p->conn_hdl);
			con->set_message_handler(&gameOnMessage);
		}
		new std::thread(gameThread); // leak the shit out of it yooo
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
	if (gameState.phase == GamePhase::GAME && gameState.players.size() == 0)
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

int main()
{
	boost::property_tree::ini_parser::read_ini(INI_PATH, gameState.config);

	srand(time(0));

	websocket_server.get_alog().clear_channels(websocketpp::log::alevel::all);

	websocket_server.set_open_handler(&on_open);
	websocket_server.set_message_handler(&on_message);
	websocket_server.set_reuse_addr(true);
	websocket_server.set_close_handler(&on_close);

	websocket_server.init_asio();
	int port = gameState.config.get<int>("default.port");
	websocket_server.listen(port);
	websocket_server.start_accept();

	std::cout << "server started on port " << port << "\n";

	websocket_server.run();

	std::cout << "server stopped\n";
	return 0;
}
