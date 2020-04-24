#include <iostream>

#include "lobbystate.hpp"
#include "gamedata.hpp"

void LobbyWebSocketListener(std::string& data) {
    std::cout << "lobby received data\n";
}

void LobbyState::Create() {
    std::cout << "lobby create\n";
    std::cout << "server " << gameData.serverAddress << ", my nickname " << gameData.myNickname << "\n";
    this->socket = CreateWebSocket();
    this->socket->listener = &LobbyWebSocketListener;
    int ret = this->socket->Connect(gameData.serverAddress);
    if (ret < 0)
        std::cout << "socket->Connect failed " << ret << "\n";
    else
        std::cout << "socket connected\n";
}

void LobbyState::Update() {

}

void LobbyState::CleanUp() {

}