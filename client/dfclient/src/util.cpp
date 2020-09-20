#include <ncine/TextNode.h>

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

std::vector<nc::Vector2f> createArcTexels(float outerRadius, float innerRadius, int degrees)
{
	std::vector<nc::Vector2f> ret;
	// start from left
	nc::Vector2f inner = {-innerRadius, 0};
	nc::Vector2f outer = {-outerRadius, 0};
	for (int i = 0; i < 2 * degrees + 1; i++) {
		ret.push_back(outer);
		ret.push_back(inner);
		outer = rotateVector(outer, M_PI / 360);
		inner = rotateVector(inner, M_PI / 360);
	}
	return ret;
}

nc::MeshSprite* createArc(nc::SceneNode& rootNode, nc::Texture* texture,
	float x, float y, float outerRadius, float innerRadius, int degrees) {
	auto ret = new nc::MeshSprite(&rootNode, texture, x, y);
	auto texels = createArcTexels(outerRadius, innerRadius, degrees);
	ret->createVerticesFromTexels(texels.size(), texels.data());
	ret->setAnchorPoint(-100, -100); // for the love of god i have no idea why this should be like that
	return ret;
}

tweeny::tween<int> CreateTextTween(ncine::TextNode* textPtr) {
	auto tween = tweeny::from(255)
		.to(255).during(60)
		.to(0).during(60).onStep(
		[textPtr] (tweeny::tween<int>& t, int v) -> bool {
			auto textColor = textPtr->color();
			textPtr->setColor(textColor.r(), textColor.g(), textColor.b(), v);
			return false;
		}
	);
	return std::move(tween);
}
