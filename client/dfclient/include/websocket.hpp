#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <functional>

#include "messages.hpp"

struct WebSocket {
	virtual int Connect(std::string& address) = 0;
	virtual bool Send(std::string& data) = 0;
	virtual void Close() = 0;
	virtual ~WebSocket() {}

	// TODO use lock-free queue instead?
	std::mutex mq_mutex;
	bool toBeOpened = false;
	std::vector<std::string> messageQueue;
};

struct WebSocketManager {
	Messages GetMessages();

	std::unique_ptr<WebSocket> _ws;
};

void CreateWebSocket();

extern WebSocketManager webSocketManager;
