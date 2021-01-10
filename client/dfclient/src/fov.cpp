#include <cassert>
#include <cmath>
#include <set>
#include <queue>
#include <functional>

#include "fov.hpp"

namespace fov {

static std::vector<segment> deoverlap(ncine::Vector2f origin, std::vector<segment> segments);

inline static double cross(ncine::Vector2f a, ncine::Vector2f b) {
	return a.x * b.y - a.y * b.x;
}

inline static double dot(ncine::Vector2f a, ncine::Vector2f b) {
	return a.x * b.x + a.y * b.y;
}

static bool isOrientedClockwise(const chain& c) {
	if (c.empty()) {
		return true;
	}

	// Calculate the signed area of the polygon bounded by the chain.
	// Assumption: there are no self-intersections in the chain.
	float area = 0;
	ncine::Vector2f prev = c.back();
	for (ncine::Vector2f curr : c) {
		area += cross(prev, curr);
		prev = curr;
	}

	// In this convention, the polygonal chain is CW iff the signed area >= 0.f.
	return area >= 0.f;
}

// Adapted from Tim Voght's implementation of Paul Bourke's circle intersection
// algorithm:
// http://paulbourke.net/geometry/circlesphere/tvoght.c
// Returns the intersection points of the circles. Always returns at most
// two points.
// `out` must point to an at-least-two-element array.
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

	for (const auto& seg : _segments) {
		// Optimization: include the segment in the outline iff it is
		// front facing towards the origin. Assuming that the segments
		// originate from simple polygons, the front-facing segments
		// will always obscure the back-facing segments. This reduces
		// the number of segments to be processed in the deoverlapping phase,
		// and does not affect the outcome.
		if (cross(seg.second - seg.first, origin - seg.first) < 0.f) {
			ret.push_back(seg);
		}
	}

	for (const auto& c : _circles) {
		// The circle is represented by a line segment between points
		// formed by intersection of rays tangential to the circle,
		// with the circle. The resulting shadow outside the area of the
		// circle will have exactly the same shape as the shadow cast by
		// a real circle would have.
		circle intersectionC;
		intersectionC.pos = (origin + c.pos) * 0.5f;
		intersectionC.radius = (origin - c.pos).length() * 0.5f;

		ncine::Vector2f pts[2];
		if (circleIntersection(intersectionC, c, pts) == 2) {
			ret.emplace_back(pts[0], pts[1]);
		}
	}

	return deoverlap(origin, std::move(ret));
}

struct event {
	enum class type {
		removeSegment = 0,
		addSegment = 1,
	};

	// The angle used to order the events.
	double angle;

	// Squared distance to the origin.
	double sqDist;

	// Event type.
	type typ;

	// ID of the segment associated with this event.
	int segmentID;

	inline bool operator>(const event& other) const {
		if (angle != other.angle) {
			return angle > other.angle;
		}
		if (sqDist != other.sqDist) {
			return sqDist > other.sqDist;
		}

		if (typ != other.typ) {
			return (int)typ > (int)other.typ;
		}

		return segmentID > other.segmentID;
	}
};

// Calculates the intersection between the ray starting from `origin`
// and crossing `origin` + `d` with the segment `seg`. If `treatSegmentAsLine`
// is true, the check instead checks if the ray intersects with a line which
// goes through both endpoints of the `seg`.
//
// If objects do not overlap, returns a negative number. Otherwise returns
// a number which represents the distance from `origin` to the intersection
// point divided by the length of `d`.
float raySegmentTest(ncine::Vector2f origin, ncine::Vector2f d, segment seg, bool treatSegmentAsLine = false) {
	// https://rootllama.wordpress.com/2014/06/20/ray-line-segment-intersection-test-in-2d/
	const auto v1 = origin - seg.first;
	const auto v2 = seg.second - seg.first;
	const auto v3 = ncine::Vector2f(-d.y, d.x);

	const auto dp = dot(v2, v3);
	if (fabs(dp) < 0.001f) {
		return -1.0f;
	}

	const auto t2 = dot(v1, v3) / dp;
	if (!treatSegmentAsLine && (t2 < 0.0f || t2 > 1.0f)) {
		return -1.0f;
	}

	return cross(v2, v1) / dp;
}

// The algorithm takes an origin point and a list of segments. It returns
// another list of segments which consists only of those parts of the original
// segments which are visible from the origin point.
//
// A queue of events is prepared first: for each segment in the list, add
// an addSegment event and an removeSegment, each of them associated with
// an appropriate end of the segment. The events are processed in angular
// order based on the origin point.
//
// For a better understanding of the algorithm, imagine a ray being shot
// from the origin point horizontally, towards +X. During the execution
// of the algorithm, the ray performs one full rotation around the origin point.
// While rotating, if the ray touches a point associated with an event,
// the event is processed.
//
// At all times, a structure called "broom" is kept. It contains all segments
// from the list which cross the ray at the given moment. Segments are sorted
// with respect to the distance to the broom; here, the distance means
// the distance of the ray-segment intersection point from the origin.
//
// TODO: Detect crossing segments. Running Bentley-Ottmann algorithm
// before running the current one should suffice.
static std::vector<segment> deoverlap(ncine::Vector2f origin, std::vector<segment> segments) {
	struct segmentRef { int segmentID; };

	auto segmentCompare = [&] (const segmentRef& a, const segmentRef& b) -> bool {
		// Assumption: segments being compared do not overlap. Therefore,
		// for all rays from the origin which intersect both segments,
		// we will always either have intersection point of `a` being closer
		// to the origin than `b`, or vice versa. This means we can choose
		// _any_ ray from the origin to compare both segments.
		//
		// As of now, the assumption may not always be correct, as nothing
		// prevents the input segments from crossing each other. For now,
		// the algorithm seems to be working well, with noticeable but rare
		// glitches. To fix this properly, we will have to run a Bentley-Ottmann
		// pass before calling the `deoverlap` function.

		const auto& segA = segments[a.segmentID];
		const auto& segB = segments[b.segmentID];

		// Check if the ray (origin, segA.first) intersects segB
		auto d = raySegmentTest(origin, segA.first - origin, segB);
		if (d >= 0.f) {
			// The ray going through `segA.first` intersects both segments,
			// we need to determine which segment is closer wrt this ray.
			//
			// Notice that we chose the ray so that the intersection point
			// of the ray and `segA` is precisely `segA.first`.
			//
			// Now, the result of `raySegmentTest` is the distance from
			// the `origin` to the intersection point of the ray with `segB`,
			// divided by |segA.first - origin|. Coincidentally, the distance
			// from the `origin` to the intersection point of the ray
			// with `segA` is precisely |segA.first - origin|. Therefore,
			// if `segB` is further than `segA`, then `d` > 1.f, and `d` <= 1.f
			// otherwise.
			return d > 1.f;
		}

		d = raySegmentTest(origin, segB.first - origin, segA);
		if (d >= 0.f) {
			// The situation is analogous to the one explained in the comment
			// above, but the segments are swapped, therefore the comparison
			// of the `d` with 1.f is inverted.
			return d < 1.f;
		}

		return false;
	};

	std::vector<segment> ret;

	std::priority_queue<event, std::vector<event>, std::greater<event>> evts;
	std::set<segmentRef, decltype(segmentCompare)> broom{segmentCompare};

	// Push those segments to the queue which are overlapping with a ray
	// starting from the origin, and going to the right (increasing X)
	for (int i = 0; i < segments.size(); i++) {
		const auto addEvt = [&] (ncine::Vector2f pt, event::type typ, int segmentID) {
			const auto d = pt - origin;
			auto a = -atan2(d.y, d.x);
			if (a < 0.f) {
				a += 2.f * M_PI;
			}
			evts.push(event {
				a,
				d.sqrLength(),
				typ,
				segmentID
			});
			return a;
		};

		const auto& seg = segments[i];
		const auto ang1 = addEvt(seg.first, event::type::addSegment, i);
		const auto ang2 = addEvt(seg.second, event::type::removeSegment, i);
		if (ang1 > ang2) {
			broom.insert(segmentRef { i });
		}
	}

	// Calculates the intersection point of a ray from origin going through
	// the line (segment) `seg`.
	auto shoot = [&] (ncine::Vector2f through, segment seg) -> ncine::Vector2f {
		const auto d = raySegmentTest(origin, (through - origin), seg, true);
		return origin + (through - origin) * d;
	};

	// prevMin holds the index of the first segment which intersects the ray.
	// The ray will "slide" along this segment until it reaches its end,
	// or it becomes obscured by another one.
	int prevMin = broom.empty() ? -1 : broom.begin()->segmentID;

	// prevPt holds a point which will be used as a beginning of the next
	// segment to include in the result list.
	// It's valid iff prevMin != -1
	ncine::Vector2f prevPt;
	if (prevMin != -1) {
		prevPt = shoot(origin + ncine::Vector2f(1.f, 0.f), segments[prevMin]);
	}

	// Process events in the queue
	while (!evts.empty()) {
		const auto evt = evts.top();
		evts.pop();

		// Update the broom according to the event, and get the point
		// associated with the event
		ncine::Vector2f pt;
		if (evt.typ == event::type::addSegment) {
			broom.insert(segmentRef { evt.segmentID });
			pt = segments[evt.segmentID].first;
		} else {
			broom.erase(segmentRef { evt.segmentID });
			pt = segments[evt.segmentID].second;
		}

		// currMin holds the intex of the first segment which NOW intersects
		// the ray
		int currMin = broom.empty() ? -1 : broom.begin()->segmentID;

		if (prevMin != currMin) {
			// After processing the event, the ID of the segment being
			// first on the broom has changed. This means that either
			// the ray encountered the end of the segment, a new segment
			// started obscuring the segment which was previously the first,
			// or the broom was empty and a new segment just appeared.
			//
			// In either case, we need to stop "sliding" on the previous
			// line segment (if the broom wasn't empty), and start sliding
			// on the new line segment (if the room isn't empty).

			if (prevMin != -1) {
				// Put a visible part of the line segment we were previously
				// sliding on.
				ret.push_back(segment {
					prevPt,
					shoot(pt, segments[prevMin])
				});
			}

			if (currMin != -1) {
				// Set the starting point of the next segment to be put into
				// the result set as the current intersection point between
				// the current ray and the new first line segment in the broom.
				// When either the segment ends or becomes obscured,
				// this point will be used to produce a visible part
				// of the original line segment.
				prevPt = shoot(pt, segments[currMin]);
			}
		} else {
			// The ID of the first segment on the broom didn't change.
			// Changes made by the event to the broom are invisible
			// from the origin. We may continue "sliding" along the current
			// segment (if there are any segment on the broom).
		}

		prevMin = currMin;
	}

	if (prevMin != -1) {
		// At the start of the algorith, we set the ray to be horizontal
		// and point in the +X direction. The broom might contain some segments
		// in the beginning, therefore it might already intersect with a visible
		// part of the first segment. We will start by outputting the fragment
		// of this part which is below the ray.
		//
		// Here, we made a full circle and are ready to output the remaining
		// fragment of the first visible part.
		ret.push_back(segment {
			prevPt,
			shoot(origin + ncine::Vector2f(1.f, 0.f), segments[prevMin])
		});
	}

	return ret;
}

}
