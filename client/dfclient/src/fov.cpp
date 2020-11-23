#include "fov.hpp"

namespace fov {

inline static double cross(ncine::Vector2f a, ncine::Vector2f b) {
	return a.x * b.y - a.y * b.x;
}

static bool isOrientedClockwise(const chain& c) {
	if (c.empty()) {
		return true;
	}

	float area = 0;
	ncine::Vector2f prev = c.back();
	for (ncine::Vector2f curr : c) {
		area += cross(prev, curr);
		prev = curr;
	}

	return area >= 0.f;
}

// Adapted from Tim Voght's implementation of Paul Bourke's circle intersection
// algorithm:
// http://paulbourke.net/geometry/circlesphere/tvoght.c
// `out` must point to two-element array
static size_t circleIntersection(const circle& a, const circle& b, ncine::Vector2f* out) {
	const auto d = b.pos - a.pos;
	const auto dist = d.length();
	if (dist > a.radius + b.radius) {
		// Circles are disjoint
		return 0;
	}
	if (dist < abs(a.radius - b.radius)) {
		// One circle is contained in the other
		return 0;
	}

	const float p = ((a.radius * a.radius) - (b.radius * b.radius) + dist * dist)
			/ (2.f * dist);
	const auto p2 = a.pos + d * (p / dist);
	const auto h = sqrtf((a.radius * a.radius) - (p * p));
	const auto r = ncine::Vector2f(-d.y, d.x) * (h / dist);
	out[0] = p2 + r;
	out[1] = p2 - r;
	return 2;
}

void mesh::addChain(const chain& c) {
	const bool cw = isOrientedClockwise(c);
	ncine::Vector2f prev = c.back();
	for (ncine::Vector2f curr : c) {
		if (cw) {
			_segments.emplace_back(prev, curr);
		} else {
			_segments.emplace_back(curr, prev);
		}
		prev = curr;
	}
}

void mesh::addChain(const circle& c) {
	_circles.push_back(c);
}

std::vector<segment> mesh::calculateOutline(ncine::Vector2f origin) const {
	std::vector<segment> ret;

	// Dumb implementation for now: render all segments which are oriented well
	for (const auto& seg : _segments) {
		if (cross(seg.second - seg.first, origin - seg.first) < 0.f) {
			ret.push_back(seg);
		}
	}

	for (const auto& c : _circles) {
		circle intersectionC;
		intersectionC.pos = (origin + c.pos) * 0.5f;
		intersectionC.radius = (origin - c.pos).length() * 0.5f;

		ncine::Vector2f pts[2];
		if (circleIntersection(intersectionC, c, pts) == 2) {
			ret.emplace_back(pts[0], pts[1]);
		}
	}

	return ret;
}

}
