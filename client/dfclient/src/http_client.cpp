#include "http_client.hpp"

#include <exception>

#ifdef __EMSCRIPTEN__
// TODO: write an emscripten version
int http::MatchmakerGet(std::string address, std::string port, std::string& ret)
{
	return -1;
}
#else // __EMSCRIPTEN__

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace bhttp = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

static int throwingMatchmakerGet(std::string address, std::string port, std::string& ret)
{
	// The io_context is required for all I/O
	net::io_context ioc;

	// These objects perform our I/O
	tcp::resolver resolver(ioc);
	beast::tcp_stream stream(ioc);

	// Look up the domain name
	auto const results = resolver.resolve(address, port);

	// Make the connection on the IP address we get from a lookup
	stream.connect(results);

	// Set up an HTTP GET request message
	bhttp::request<bhttp::string_body> req{bhttp::verb::get, "/matchmake", 10};
	req.set(bhttp::field::host, address);
	req.set(bhttp::field::user_agent, BOOST_BEAST_VERSION_STRING);

	// Send the HTTP request to the remote host
	bhttp::write(stream, req);

	// This buffer is used for reading
	beast::flat_buffer buffer;

	// Declare a container to hold the response
	bhttp::response<bhttp::dynamic_body> res;

	// Receive the HTTP response
	bhttp::read(stream, buffer, res);

	// Gracefully close the socket
	beast::error_code ec;
	stream.socket().shutdown(tcp::socket::shutdown_both, ec);

	if(ec) {
		std::cout << "socket failed, ec: " << ec << "\n";
		return -1;
	}

	ret = beast::buffers_to_string(res.body().data());

	return res.result_int();
}

int http::MatchmakerGet(std::string address, std::string port, std::string& ret)
{
	int retCode = 0;
	try {
		retCode = throwingMatchmakerGet(address, port, ret);
	} catch (std::exception& e) {
		std::cout << "failed to connect to matchmaker: " << e.what() << "\n";
		return -1;
	}
	return retCode;
}

#endif // __EMSCRIPTEN__
