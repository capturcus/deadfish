#include "websocket.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/websocket.h>

class WebSocketEmscripten 
    : public WebSocket
{
public:
    int Connect(std::string& address) override;
    bool Send(std::string& data) override;
    virtual ~WebSocketEmscripten() {}

    EMSCRIPTEN_WEBSOCKET_T _socket;
};

std::unique_ptr<WebSocket> CreateWebSocket() {
    return std::unique_ptr<WebSocket>(new WebSocketEmscripten());
}

EM_BOOL WebSocketEmscriptenOnMessage(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData) {
    WebSocketEmscripten* socket = (WebSocketEmscripten*) userData;
    if (!socket->listener)
        return false;

    std::string data = std::string(e->data, e->data + e->numBytes);
    socket->listener(data);
    return false;
}

int WebSocketEmscripten::Connect(std::string& address) {
    EmscriptenWebSocketCreateAttributes attr;
	emscripten_websocket_init_create_attributes(&attr);

	attr.url = address.data();

	EMSCRIPTEN_WEBSOCKET_T socket = emscripten_websocket_new(&attr);
    if (socket < 0)
        return socket;
    
    this->_socket = socket;

    if (emscripten_websocket_set_onmessage_callback(this->_socket, this, WebSocketEmscriptenOnMessage) !=
        EMSCRIPTEN_RESULT_SUCCESS)
        return -1;

    return 0;
}

bool WebSocketEmscripten::Send(std::string& data) {
    return emscripten_websocket_send_binary(this->_socket, data.data(), data.size()) == EMSCRIPTEN_RESULT_SUCCESS;
}

#endif // __EMSCRIPTEN__