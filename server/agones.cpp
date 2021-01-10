#include "agones.hpp"

namespace dfAgones = agones;

#include <agones/sdk.h>
#include <grpc++/grpc++.h>
#include <iostream>
#include <thread>

static std::shared_ptr<agones::SDK> sdk;

// send health check pings
static void DoHealth()
{
	while (true)
	{
		bool ok = sdk->Health();
		if (!ok) {
			std::cout << "health ping failed\n";
		}
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

// watch GameServer Updates
static void WatchUpdates()
{
	std::cout << "Starting to watch GameServer updates...\n"
			  << std::flush;
	sdk->WatchGameServer([](const agones::dev::sdk::GameServer &gameserver) {
		std::cout << "GameServer Update:\n"
				  << "\tname: " << gameserver.object_meta().name() << "\n"
				  << "\tstate: " << gameserver.status().state() << "\n"
				  << std::flush;
	});
}

bool dfAgones::Start()
{
	std::cout << "C++ Game Server has started!\n"
			  << "Getting the instance of the SDK.\n"
			  << std::flush;
	sdk = std::make_shared<agones::SDK>();

	std::cout << "Attempting to connect...\n"
			  << std::flush;
	if (!sdk->Connect())
		return false;

	std::cout << "...handshake complete.\n"
			  << std::flush;

	std::cout << "Marking server as ready...\n"
			  << std::flush;
	auto status = sdk->Ready();
	if (!status.ok())
	{
		std::cerr << "Could not run Ready(): " << status.error_message()
				  << ". Exiting!\n";
		return false;
	}
	std::cout << "...marked Ready\n"
			  << std::flush;

	status = sdk->SetLabel("playing", "false");
	if (!status.ok()) {
		std::cout << "failed to set playing to true\n";
		return false;
	}

	std::cout << "Getting GameServer details...\n"
			  << std::flush;
	agones::dev::sdk::GameServer gameserver;
	status = sdk->GameServer(&gameserver);

	if (!status.ok())
	{
		std::cerr << "Could not run GameServer(): " << status.error_message()
				  << ". Exiting!\n";
		return false;
	}

	std::cout << "GameServer name: " << gameserver.object_meta().name() << "\n"
			  << std::flush;

	// leak those
	new std::thread(DoHealth);
	new std::thread(WatchUpdates);

	return true;
}

void dfAgones::Shutdown() {
	sdk->Shutdown();
}

void dfAgones::SetPlayers(int players) {
	auto status = sdk->SetLabel("players", std::to_string(players));
	if (!status.ok())
		std::cout << "failed to set players: " << players << "\n";
	else
		std::cout << "set label players to " << players << "\n";
}

void dfAgones::SetPlaying() {
	auto status = sdk->SetLabel("playing", "true");
	if (!status.ok())
		std::cout << "failed to set playing to true\n";
	else
		std::cout << "set playing to true\n";
}
