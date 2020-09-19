#include "websocket.hpp"

WebSocketManager webSocketManager;

// Runs in game loop thread
Messages WebSocketManager::GetMessages() {
	Messages m;
	if (!_ws) {
		return m;
	}

	std::lock_guard<std::mutex> guard(_ws->mq_mutex);
	m.opened = _ws->toBeOpened;
	m.data_msgs.swap(_ws->messageQueue);

	if (m.opened) {
		_ws->toBeOpened = false;
	}

	return m;
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
	std::lock_guard<std::mutex> guard(wspp->mq_mutex);
	socket->messageQueue.push_back(std::string(e->data, e->data + e->numBytes));
	return false;
}

EM_BOOL WebSocketEmscriptenOnOpen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData) {
	WebSocketEmscripten* socket = (WebSocketEmscripten*) userData;
	std::lock_guard<std::mutex> guard(socket->mq_mutex);
	socket->toBeOpened = true;
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
	std::lock_guard<std::mutex> guard(wspp->mq_mutex);
	wspp->messageQueue.push_back(msg->get_payload());
}

void WebSocketPPOnOpen(websocketpp::connection_hdl hdl) {
	WebSocketPP* wspp = clients[hdl.lock().get()];
	std::lock_guard<std::mutex> guard(wspp->mq_mutex);
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

	// FIXME return bool
}

#endif

WebSocket* CreateWebSocket() {
	webSocketManager._ws = std::make_unique<WebSocketType>();
	return webSocketManager._ws.get();
}
