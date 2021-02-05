#pragma once

#include <string>

// TODO: support both wasm and native builds

namespace http {

/**
 * Returns a http code if succeeded, otherwise -1
 * */
int MatchmakerGet(std::string address, std::string port, std::string& ret);

};
