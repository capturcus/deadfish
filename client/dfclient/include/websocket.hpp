#pragma once
#include <string>
#include <memory>

typedef void (*WebSocketListener)(std::string& data);
typedef void (*WebSocketCallback)();

struct WebSocket {
    virtual int Connect(std::string& address) = 0;
    virtual bool Send(std::string& data) = 0;
    virtual ~WebSocket() {}
    WebSocketListener onMessage = nullptr;
    WebSocketCallback onOpen = nullptr;
};

std::unique_ptr<WebSocket> CreateWebSocket();
