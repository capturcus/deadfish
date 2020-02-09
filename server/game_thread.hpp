void gameThread();
uint16_t newID();
void gameOnMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
bool operator==(websocketpp::connection_hdl &a, websocketpp::connection_hdl &b);
void spawnPlayer(Player *const p);
void spawnCivilian();
Player *const getPlayerByConnHdl(websocketpp::connection_hdl &hdl);
void executeKill(Player* p, Mob* m);
void sendServerMessage(Player *const player,
    flatbuffers::FlatBufferBuilder &builder,
    DeadFish::ServerMessageUnion type,
    flatbuffers::Offset<void> offset);
std::string makeServerMessage(flatbuffers::FlatBufferBuilder &builder,
    DeadFish::ServerMessageUnion type,
    flatbuffers::Offset<void> offset);