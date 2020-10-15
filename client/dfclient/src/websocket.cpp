#include "websocket.hpp"

#include <iostream>

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
	std::lock_guard<std::mutex> guard(socket->mq_mutex);
	socket->messageQueue.push_back(std::string(e->data, e->data + e->numBytes));
	return false;
}

EM_BOOL WebSocketEmscriptenOnOpen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData) {
	WebSocketEmscripten* socket = (WebSocketEmscripten*) userData;
	std::lock_guard<std::mutex> guard(socket->mq_mutex);
	socket->toBeOpened = true;
	return false;
}

EM_BOOL WebSocketEmscriptenOnError(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData) {
	std::cout << "WebSocketEmscriptenOnError\n";
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
	
	if (emscripten_websocket_set_onerror_callback(this->_socket, this, WebSocketEmscriptenOnError) !=
		EMSCRIPTEN_RESULT_SUCCESS)
		return -1;
	
	return 0;
}

bool WebSocketEmscripten::Send(std::string& data) {
	return emscripten_websocket_send_binary(this->_socket, data.data(), data.size()) == EMSCRIPTEN_RESULT_SUCCESS;
}

#else // __EMSCRIPTEN__

class WebSocketBeast
	: public WebSocket
{
public:
	int Connect(std::string& address) override;
	bool Send(std::string& data) override;
	virtual ~WebSocketBeast() {}

};

typedef WebSocketBeast WebSocketType;

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

net::io_context ioc;
websocket::stream<tcp::socket> ws{ioc};
beast::flat_buffer buffer;

void DoRead();

void OnRead(beast::error_code ec, std::size_t bytes_transferred) {
	if(ec == websocket::error::closed)
		return;

	if(ec) {
		if (ec.value() == boost::system::errc::operation_canceled) {
			// this websocket has disconnected
			// todo: cleanup whatever is happening and return to main menu
			return;
		}
		std::cout << "read failed " << ec << "\n";
	}

	auto str = beast::buffers_to_string(buffer.data());
	WebSocketBeast* wsb = (WebSocketBeast*) webSocketManager._ws.get();
	std::lock_guard<std::mutex> guard(wsb->mq_mutex);
	wsb->messageQueue.push_back(str);
	buffer.consume(buffer.size());
	DoRead();
}

void DoRead() {
	ws.async_read(buffer, &OnRead);
}

int WebSocketBeast::Connect(std::string& fullAddress) {
	tcp::resolver resolver{ioc};

	ws.binary(true);

	auto address = fullAddress.substr(5, fullAddress.size());

	auto colon = address.find(":");
	if (colon == std::string::npos)
		return -1;

	// some boost websocket stuff
	auto host = address.substr(0, colon);
	auto port = address.substr(colon + 1, address.size());
	auto const results = resolver.resolve(host, port);
	auto ep = net::connect(ws.next_layer(), results);
	host += ':' + std::to_string(ep.port());
	ws.set_option(websocket::stream_base::decorator(
		[](websocket::request_type& req)
		{
			req.set(http::field::user_agent,
				std::string(BOOST_BEAST_VERSION_STRING) +
					" websocket-client-coro");
		}));
	ws.handshake(host, "/");

	webSocketManager._ws->toBeOpened = true;

	DoRead();

	new std::thread([&](){
		ioc.run();
	});

	return 0;
}

bool WebSocketBeast::Send(std::string& data) {
	ws.write(net::buffer(std::string(data)));
}

#endif

WebSocket* CreateWebSocket() {
	webSocketManager._ws = std::make_unique<WebSocketType>();
	return webSocketManager._ws.get();
}
