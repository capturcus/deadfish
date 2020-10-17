#pragma once

void gameThread();
uint16_t newMobID();
void gameOnMessage(dfws::Handle hdl, const std::string& msg);
void spawnPlayer(Player& p);
void spawnCivilian();
Player* getPlayerByConnHdl(dfws::Handle hdl);
void sendServerMessage(Player& player,
	flatbuffers::FlatBufferBuilder &builder,
	FlatBuffGenerated::ServerMessageUnion type,
	flatbuffers::Offset<void> offset);
std::string makeServerMessage(flatbuffers::FlatBufferBuilder &builder,
	FlatBuffGenerated::ServerMessageUnion type,
	flatbuffers::Offset<void> offset);
bool mobSeePoint(Mob &m, b2Vec2 &point);
void sendToAll(std::string &data);
void sendHighscores();
void sendGameAlreadyInProgress(dfws::Handle hdl);
