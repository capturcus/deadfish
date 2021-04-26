#pragma once

#include <string>

#include <boost/asio/ip/tcp.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace dfws {

typedef int Handle;

typedef void (*OnMessageHandler) (Handle hdl, const std::string& msg);
typedef void (*OnOpenHandler) (Handle hdl);
typedef void (*OnCloseHandler) (Handle hdl);

void SendData(Handle hdl, const std::string& data);
void SetOnMessage(OnMessageHandler msgHandler);
void SetOnOpen(OnOpenHandler handler);
void SetOnClose(OnCloseHandler handler);
void Run(unsigned short port);
void Close(Handle hdl);

}
