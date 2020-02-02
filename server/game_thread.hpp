void gameThread();
uint16_t newID();
void gameOnMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
bool operator==(websocketpp::connection_hdl &a, websocketpp::connection_hdl &b);
void spawnPlayer(Player *const p);
Player *const getPlayerByConnHdl(websocketpp::connection_hdl &hdl);
void executeKill(Player* p, Mob* m);

void spawnCivilian();