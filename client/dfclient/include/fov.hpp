#pragma once

#include <vector>
#include <ncine/Vector2.h>

namespace fov {

/// A sequence of points which form a light-blocking chain.
using chain = std::vector<ncine::Vector2f>;
using segment = std::pair<ncine::Vector2f, ncine::Vector2f>;

struct circle {
	ncine::Vector2f pos;
	float radius;
};

/// A mesh describing light-blocking parts of the level.
class mesh {
private:
	std::vector<segment> _segments;
	std::vector<circle> _circles;

public:
	void addChain(const chain& c);
	void addChain(const circle& c);
	std::vector<segment> calculateOutline(ncine::Vector2f origin) const;
};

}
