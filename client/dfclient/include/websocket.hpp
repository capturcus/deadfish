#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <functional>

#include "messages.hpp"

struct WebSocket {
	virtual int Connect(std::string& address) = 0;
	virtual bool Send(const std::string& data) = 0;
	virtual void Close() = 0;
	virtual ~WebSocket() {}

	// TODO use lock-free queue instead?
	std::mutex mq_mutex;
	bool toBeOpened = false;
	std::vector<std::string> messageQueue;
};

Messages GetMessages();

void CreateWebSocket();

extern std::unique_ptr<WebSocket> GlobalWebsocket;
