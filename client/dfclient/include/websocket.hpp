#pragma once
#include <string>
#include <memory>
#include <functional>

struct WebSocket {
    virtual int Connect(std::string& address) = 0;
    virtual bool Send(std::string& data) = 0;
    virtual ~WebSocket() {}
    std::function<void(const std::string&)> onMessage = nullptr;
    std::function<void()> onOpen = nullptr;
};

std::unique_ptr<WebSocket> CreateWebSocket();
