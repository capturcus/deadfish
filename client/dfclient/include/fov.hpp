#pragma once

#include <vector>
#include <ncine/Vector2.h>

namespace fov {

/// A sequence of points which for a light-blocking chain.
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
    void add_chain(const chain& c);
    void add_circle(const circle& c);
    std::vector<segment> calculate_outline(ncine::Vector2f origin) const;
};

}
