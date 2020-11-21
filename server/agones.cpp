#include "agones.hpp"

#include <agones/sdk.h>
#include <grpc++/grpc++.h>
#include <iostream>
#include <thread>

static std::shared_ptr<agones::SDK> sdk;

// send health check pings
void DoHealth()
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
void WatchUpdates()
{
	std::cout << "Starting to watch GameServer updates...\n"
			  << std::flush;
	sdk->WatchGameServer([](const agones::dev::sdk::GameServer &gameserver) {
		std::cout << "GameServer Update:\n"								   //
				  << "\tname: " << gameserver.object_meta().name() << "\n" //
				  << "\tstate: " << gameserver.status().state() << "\n"
				  << std::flush;
	});
}

void agonesThread()
{
	std::cout << "C++ Game Server has started!\n"
			  << "Getting the instance of the SDK.\n"
			  << std::flush;
	sdk = std::make_shared<agones::SDK>();

	std::cout << "Attempting to connect...\n"
			  << std::flush;
	if (!sdk->Connect())
	{
		std::cerr << "Exiting!\n";
		exit(1);
	}
	std::cout << "...handshake complete.\n"
			  << std::flush;

	std::thread health(DoHealth);
	std::thread watch(WatchUpdates);

	std::cout << "Setting a label\n"
			  << std::flush;
	grpc::Status status = sdk->SetLabel("test-label", "test-value");
	if (!status.ok())
	{
		std::cerr << "Could not run SetLabel(): " << status.error_message()
				  << ". Exiting!\n";
		exit(1);
	}

	std::cout << "Setting an annotation\n"
			  << std::flush;
	status = sdk->SetAnnotation("test-annotation", "test value");
	if (!status.ok())
	{
		std::cerr << "Could not run SetAnnotation(): " << status.error_message()
				  << ". Exiting!\n";
		exit(1);
	}

	std::cout << "Marking server as ready...\n"
			  << std::flush;
	status = sdk->Ready();
	if (!status.ok())
	{
		std::cerr << "Could not run Ready(): " << status.error_message()
				  << ". Exiting!\n";
		exit(1);
	}
	std::cout << "...marked Ready\n"
			  << std::flush;

	std::cout << "Getting GameServer details...\n"
			  << std::flush;
	agones::dev::sdk::GameServer gameserver;
	status = sdk->GameServer(&gameserver);

	if (!status.ok())
	{
		std::cerr << "Could not run GameServer(): " << status.error_message()
				  << ". Exiting!\n";
		exit(1);
	}

	std::cout << "GameServer name: " << gameserver.object_meta().name() << "\n"
			  << std::flush;

	health.join();
	watch.join();
}

void agonesShutdown() {
	sdk->Shutdown();
}