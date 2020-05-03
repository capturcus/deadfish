#include <iostream>

#include <ncine/Sprite.h>
#include <flatbuffers/flatbuffers.h>

#include "websocket.hpp"

bool IntersectsNode(float x, float y, ncine::DrawableNode& node);
void SendData(flatbuffers::FlatBufferBuilder& builder);

namespace deadfish {

template<typename T, typename U>
int norm(T a, U b) {
    float x = a.x - b.x;
    float y = a.y - b.y;
    return x*x + y*y;
}

}

template<typename T>
std::ostream& operator<<(std::ostream& os, const ncine::Vector2<T>& t) {
    os << t.x << "," << t.y;
    return os;
}