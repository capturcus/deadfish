#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <map>

#include <boost/beast/core/buffers_to_string.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>

#include "websocket.hpp"
#include "deadfish.hpp"

class DfWebsocket;

net::io_context ioc{1};
tcp::acceptor acceptor{ioc};
uint16_t lastSocketID = 0;
std::vector<std::shared_ptr<DfWebsocket>> sockets;

static dfws::OnMessageHandler onMessageHandler = nullptr;
static dfws::OnOpenHandler onOpenHandler = nullptr;
static dfws::OnCloseHandler onCloseHandler = nullptr;

void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

void dfws::SetOnMessage(OnMessageHandler h)
{
    onMessageHandler = h;
}

void dfws::SetOnOpen(OnOpenHandler h)
{
    onOpenHandler = h;
}

void dfws::SetOnClose(OnCloseHandler h)
{
    onCloseHandler = h;
}

class DfWebsocket : public std::enable_shared_from_this<DfWebsocket> {
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

public:
    uint16_t socketID_;

    // Take ownership of the socket
    explicit
    DfWebsocket(tcp::socket&& socket, uint16_t socketID)
        : ws_(std::move(socket)), socketID_(socketID)
    {
        ws_.binary(true);
    }

    void start() {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
            }));
        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &DfWebsocket::OnAccept,
                shared_from_this()));
    }

    void DoRead() {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &DfWebsocket::OnRead,
                shared_from_this()));
    }

    void OnAccept(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "accept");

        onOpenHandler(socketID_);

        DoRead();
    }

    void
    OnRead(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec) {
            if (ec.value() == boost::system::errc::operation_canceled) {
                // this websocket has disconnected
                // TODO delete it from the vector
                onCloseHandler(socketID_);
                return;
            }
            fail(ec, "read");
        }

        auto str = beast::buffers_to_string(buffer_.data());
        onMessageHandler(socketID_, str);
        buffer_.consume(buffer_.size());

        DoRead();
    }

    void Send(const std::string& data) {
        // sync write
        ws_.write(net::buffer(data));
    }

    void Close() {
        ws_.close(websocket::close_code::normal);
    }
};

std::shared_ptr<DfWebsocket> getSocketForHandle(dfws::Handle hdl)
{
    for (auto& s : sockets) {
        if (s->socketID_ == hdl) {
            return s;
        }
    }
    std::cout << "could not find socket with id " << hdl << "\n";
    exit(1);
}

void dfws::SendData(Handle hdl, const std::string& data)
{
    auto s = getSocketForHandle(hdl);
    s->Send(data);
}

void dfws::Close(Handle hdl)
{
    std::cout << "closing handle " << hdl << "\n";
    auto it = sockets.begin();
    for (; it != sockets.end(); it++) {
        if ((*it)->socketID_ == hdl) {
            break;
        }
    }
    (*it)->Close();
    sockets.erase(it);
}

void dfwsOnAccept(beast::error_code ec, tcp::socket socket)
{
    std::cout << "on accept\n";
    if (ec)
        return fail(ec, "accept");

    auto socketPtr = std::make_shared<DfWebsocket>(std::move(socket), lastSocketID++);
    sockets.push_back(socketPtr);
    sockets.back()->start();

    // accept another connection
    acceptor.async_accept(&dfwsOnAccept);
}

void dfws::Run(unsigned short port)
{
    auto const address = net::ip::make_address("0.0.0.0");
    beast::error_code ec;
    tcp::endpoint endpoint{address, port};

    // Open the acceptor
    acceptor.open(endpoint.protocol(), ec);
    if (ec)
    {
        fail(ec, "open");
        return;
    }

    // Allow address reuse
    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec)
    {
        fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    acceptor.bind(endpoint, ec);
    if (ec)
    {
        fail(ec, "bind");
        return;
    }

    // Start listening for connections
    acceptor.listen(
        net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        fail(ec, "listen");
        return;
    }

    acceptor.async_accept(&dfwsOnAccept);
    ioc.run();
    std::cout << "ioc quitting\n";
}
