#include <iostream>

#include <ncine/Sprite.h>
#include <ncine/MeshSprite.h>
#include <ncine/Texture.h>

namespace nc = ncine;

#include <flatbuffers/flatbuffers.h>

#include "../../common/geometry.hpp"

#include "websocket.hpp"

bool IntersectsNode(float x, float y, nc::DrawableNode& node);
void SendData(flatbuffers::FlatBufferBuilder& builder);
nc::MeshSprite* createArc(nc::SceneNode& rootNode, nc::Texture* texture,
	float x, float y, float outerRadius, float innerRadius, int degrees);

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