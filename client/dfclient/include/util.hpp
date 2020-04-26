#include <ncine/Sprite.h>
#include <flatbuffers/flatbuffers.h>

#include "websocket.hpp"

bool IntersectsNode(float x, float y, ncine::DrawableNode& node);
void SendData(flatbuffers::FlatBufferBuilder& builder);