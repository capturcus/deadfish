#pragma once
#include <string>
#include <memory>

typedef void (*WebSocketListener)(std::string& data);

struct WebSocket {
    virtual int Connect(std::string& address) = 0;
    virtual bool Send(std::string& data) = 0;
    virtual ~WebSocket() {}
    WebSocketListener listener = nullptr;
};

std::unique_ptr<WebSocket> CreateWebSocket();
