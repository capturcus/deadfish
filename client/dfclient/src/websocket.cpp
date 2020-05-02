#include "websocket.hpp"

WebSocketManager webSocketManager;

void WebSocketManager::Update() {
    for (auto& ws: this->websockets) {
        if (ws->toBeOpened) {
            ws->onOpen();
            ws->toBeOpened = false;
        }
        
        for (auto& msg: ws->messageQueue)
            ws->onMessage(msg);
        
        ws->messageQueue.clear();
    }
}

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

typedef WebSocketEmscripten WebSocketType;

EM_BOOL WebSocketEmscriptenOnMessage(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData) {
    WebSocketEmscripten* socket = (WebSocketEmscripten*) userData;
    if (!socket->onMessage) // TODO: see whether this hinders performance and remove it if it does
        return false;

    std::string data = std::string(e->data, e->data + e->numBytes);
    socket->onMessage(data);
    return false;
}

EM_BOOL WebSocketEmscriptenOnOpen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData) {
    WebSocketEmscripten* socket = (WebSocketEmscripten*) userData;
    if (!socket->onOpen)
        return false;

    socket->onOpen();
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

    if (emscripten_websocket_set_onopen_callback(this->_socket, this, WebSocketEmscriptenOnOpen) !=
        EMSCRIPTEN_RESULT_SUCCESS)
        return -1;

    return 0;
}

bool WebSocketEmscripten::Send(std::string& data) {
    return emscripten_websocket_send_binary(this->_socket, data.data(), data.size()) == EMSCRIPTEN_RESULT_SUCCESS;
}

#else // __EMSCRIPTEN__

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> WebSocketPPClient;

class WebSocketPP
    : public WebSocket
{
public:
    int Connect(std::string& address) override;
    bool Send(std::string& data) override;
    virtual ~WebSocketPP() {}

    WebSocketPPClient c;
    websocketpp::connection_hdl myHdl;
};

typedef WebSocketPP WebSocketType;

std::map<void*, WebSocketPP*> clients;

void WebSocketPPOnMessage(websocketpp::connection_hdl hdl, WebSocketPPClient::message_ptr msg) {
    WebSocketPP* wspp = clients[hdl.lock().get()];
    const std::string& data = msg->get_payload();
    wspp->messageQueue.push_back(data);
}

void WebSocketPPOnOpen(websocketpp::connection_hdl hdl) {
    WebSocketPP* wspp = clients[hdl.lock().get()];
    wspp->toBeOpened = true;
}

int WebSocketPP::Connect(std::string& address) {
    // Set logging to be pretty verbose (everything except message payloads)
    c.set_access_channels(websocketpp::log::alevel::none);
    c.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    c.init_asio();

    // Register our message handler
    c.set_message_handler(&WebSocketPPOnMessage);
    c.set_open_handler(&WebSocketPPOnOpen);

    websocketpp::lib::error_code ec;
    WebSocketPPClient::connection_ptr con = c.get_connection(address, ec);
    if (ec) {
        std::cout << "could not create connection because: " << ec.message() << std::endl;
        return -1;
    }

    // Note that connect here only requests a connection. No network messages are
    // exchanged until the event loop starts running in the next line.
    c.connect(con);
    this->myHdl = con;

    clients[this->myHdl.lock().get()] = this;

    new std::thread([](WebSocketPPClient *c_ptr){
        c_ptr->run();
    }, &c);

    return 0;
}

bool WebSocketPP::Send(std::string& data) {
    websocketpp::lib::error_code ec;
    this->c.send(this->myHdl, data, websocketpp::frame::opcode::binary, ec);
    if (ec)
        std::cout << "Echo failed because: " << ec.message() << std::endl;
}

#endif

WebSocket* CreateWebSocket() {
    auto ws = std::make_unique<WebSocketType>();
    webSocketManager.websockets.push_back(std::move(ws));
    return webSocketManager.websockets.back().get();
}