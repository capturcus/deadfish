#include "util.hpp"
#include "game_data.hpp"

bool IntersectsNode(float x, float y, ncine::DrawableNode& node) {
    auto size = node.absSize();
    return node.x - size.x/2 < x && 
           x < node.x + size.x/2 &&
           node.y - size.y/2 < y && 
           y < node.y + size.y/2;
}

void SendData(flatbuffers::FlatBufferBuilder& builder) {
    auto data = builder.GetBufferPointer();
    auto size = builder.GetSize();
    auto str = std::string(data, data + size);

    gameData.socket->Send(str);
}
