#pragma once

#include <string>

namespace dfws {

typedef int Handle;

typedef void (*OnMessageHandler) (Handle hdl, const std::string& msg);
typedef void (*OnOpenHandler) (Handle hdl);
typedef void (*OnCloseHandler) (Handle hdl);

void Init();
void SendData(Handle hdl, const std::string& data);
void SetOnMessage(OnMessageHandler msgHandler);
void SetOnOpen(OnOpenHandler handler);
void SetOnClose(OnCloseHandler handler);
void Run();

};