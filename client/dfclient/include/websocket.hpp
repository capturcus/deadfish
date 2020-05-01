#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>

struct WebSocket {
    virtual int Connect(std::string& address) = 0;
    virtual bool Send(std::string& data) = 0;
    virtual ~WebSocket() {}
    std::function<void(const std::string&)> onMessage = nullptr;
    std::function<void()> onOpen = nullptr;

    bool toBeOpened;
    std::vector<std::string> messageQueue;
};

struct WebSocketManager {
    void Update();

    std::vector<std::unique_ptr<WebSocket>> websockets;
};

WebSocket* CreateWebSocket();

extern WebSocketManager webSocketManager;