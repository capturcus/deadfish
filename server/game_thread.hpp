void gameThread();
uint16_t newID();
void gameOnMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
bool operator==(websocketpp::connection_hdl &a, websocketpp::connection_hdl &b);
void spawnPlayer(Player& p);
void spawnCivilian();
Player& getPlayerByConnHdl(websocketpp::connection_hdl &hdl);
void executeKill(Player& p, Mob& m);
void sendServerMessage(Player& player,
    flatbuffers::FlatBufferBuilder &builder,
    DeadFish::ServerMessageUnion type,
    flatbuffers::Offset<void> offset);
std::string makeServerMessage(flatbuffers::FlatBufferBuilder &builder,
    DeadFish::ServerMessageUnion type,
    flatbuffers::Offset<void> offset);
bool mobSeePoint(Mob &m, b2Vec2 &point);