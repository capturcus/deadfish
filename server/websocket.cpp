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

net::io_context ioc{1};
tcp::acceptor acceptor_{ioc};
uint16_t lastSocketID = 0;
std::map<uint16_t, websocket::stream<beast::tcp_stream>> sockets;
std::map<uint16_t, beast::flat_buffer> buffers;

void dfws::SendData(Handle hdl, const std::string& data)
{

}

void dfws::SetOnMessage(OnMessageHandler msgHandler)
{

}

void dfws::SetOnOpen(OnOpenHandler handler)
{

}

void dfws::SetOnClose(OnCloseHandler handler)
{

}

void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

void dfwsOnRead(uint16_t socketID, beast::error_code ec, std::size_t bytes_transferred)
{
    std::cout << "dfwsOnRead\n";
    auto socketItr = sockets.find(socketID);
    if (socketItr == sockets.end()) {
        std::cout << "could not find socket for id " << socketID << "\n";
        return;
    }
    auto bufferItr = buffers.find(socketID);
    if (bufferItr == buffers.end()) {
        std::cout << "could not find buffer for id " << socketID << "\n";
        return;
    }
    auto& ws = socketItr->second;
    auto& buffer = bufferItr->second;
    ws.async_read(buffer, std::bind(&dfwsOnRead, socketID, std::placeholders::_1, std::placeholders::_2));
}

void dfwsSocketOnAccept(uint16_t socketID, beast::error_code ec)
{
    std::cout << "socket on accept " << socketID << "\n";
    auto socketItr = sockets.find(socketID);
    if (socketItr == sockets.end()) {
        std::cout << "could not find socket for id " << socketID << "\n";
        return;
    }
    auto bufferItr = buffers.find(socketID);
    if (bufferItr == buffers.end()) {
        std::cout << "could not find buffer for id " << socketID << "\n";
        return;
    }
    auto& ws = socketItr->second;
    auto& buffer = bufferItr->second;
    ws.async_read(buffer, std::bind(&dfwsOnRead, socketID, std::placeholders::_1, std::placeholders::_2));
}

void dfwsOnAccept(beast::error_code ec, tcp::socket socket)
{
    std::cout << "on accept\n";
    // websocket::stream<beast::tcp_stream> ws_(std::move(socket));
    uint16_t currentID = lastSocketID++;
    sockets.emplace(std::make_pair(currentID, websocket::stream<beast::tcp_stream>(std::move(socket))));
    buffers.emplace(std::make_pair(currentID, beast::flat_buffer{}));
    auto itr = sockets.find(currentID);
    auto& ws_ = itr->second;

    ws_.binary(true);

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
    ws_.async_accept(std::bind(&dfwsSocketOnAccept, currentID, std::placeholders::_1));

    // accept another connection
    acceptor_.async_accept(&dfwsOnAccept);
}

void dfws::Run(unsigned short port)
{
    auto const address = net::ip::make_address("0.0.0.0");
    beast::error_code ec;
    tcp::endpoint endpoint{address, port};

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec)
    {
        fail(ec, "open");
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec)
    {
        fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec)
    {
        fail(ec, "bind");
        return;
    }

    // Start listening for connections
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if(ec)
    {
        fail(ec, "listen");
        return;
    }

    acceptor_.async_accept(&dfwsOnAccept);
    ioc.run();
}
