#include <cassert>
#include <cmath>
#include <set>
#include <queue>
#include <functional>

#include "fov.hpp"

namespace fov {

static const bool doDeoverlap = true;

static std::vector<segment> deoverlap(ncine::Vector2f origin, std::vector<segment> segments);

inline static double cross(ncine::Vector2f a, ncine::Vector2f b) {
    return a.x * b.y - a.y * b.x;
}

inline static double dot(ncine::Vector2f a, ncine::Vector2f b) {
    return a.x * b.x + a.y * b.y;
}

static bool is_cw(const chain& c) {
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

void mesh::add_chain(const chain& c) {
    const bool cw = is_cw(c);
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

void mesh::add_circle(const circle& c) {
    _circles.push_back(c);
}

std::vector<segment> mesh::calculate_outline(ncine::Vector2f origin) const {
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

    if (doDeoverlap) {
        return deoverlap(origin, std::move(ret));
    }
    return ret;
}

struct event {
    enum class type {
        remove_segment = 0,
        add_segment = 1,
    };

    double angle;
    double sqDist;
    type typ;
    int segment_id;

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

        return segment_id > other.segment_id;
    }
};

float ray_segment_test(ncine::Vector2f origin, ncine::Vector2f d, segment seg, bool treat_segment_as_line = false) {
    // https://rootllama.wordpress.com/2014/06/20/ray-line-segment-intersection-test-in-2d/
    const auto v1 = origin - seg.first;
    const auto v2 = seg.second - seg.first;
    const auto v3 = ncine::Vector2f(-d.y, d.x);

    const auto dp = dot(v2, v3);
    if (fabs(dp) < 0.001f) {
        return -1.0f;
    }

    const auto t2 = dot(v1, v3) / dp;
    if (!treat_segment_as_line && (t2 < 0.0f || t2 > 1.0f)) {
        return -1.0f;
    }

    return cross(v2, v1) / dp;
}

// The algorithm traverses points in the angular order, and keeps track
// of the segment that is the closest to the origin point at given angle.
//
// TODO: Detect crossing segments. Running Bentley-Ottmann algorithm
// before running the current one should suffice.
static std::vector<segment> deoverlap(ncine::Vector2f origin, std::vector<segment> segments) {
    struct segment_ref { int segment_id; };

    auto segment_compare = [&] (const segment_ref& a, const segment_ref& b) -> bool {
        const auto& seg_a = segments[a.segment_id];
        const auto& seg_b = segments[b.segment_id];
        auto d = ray_segment_test(origin, seg_a.first - origin, seg_b);
        if (d >= 0.f) {
            return d > 1.f;
        }

        d = ray_segment_test(origin, seg_b.first - origin, seg_a);
        if (d >= 0.f) {
            return d < 1.f;
        }

        return false;
    };

    std::vector<segment> ret;

    std::priority_queue<event, std::vector<event>, std::greater<event>> evts;
    std::set<segment_ref, decltype(segment_compare)> broom{segment_compare};

    // Push those segments to the queue which are overlapping with a ray
    // starting from the origin, and going to the right (increasing X)
    for (int i = 0; i < segments.size(); i++) {
        const auto add_evt = [&] (ncine::Vector2f pt, event::type typ, int segment_id) {
            const auto d = pt - origin;
            auto a = -atan2(d.y, d.x);
            if (a < 0.f) {
                a += 2.f * M_PI;
            }
            evts.push(event {
                a,
                d.sqrLength(),
                typ,
                segment_id
            });
            return a;
        };

        const auto& seg = segments[i];
        const auto ang1 = add_evt(seg.first, event::type::add_segment, i);
        const auto ang2 = add_evt(seg.second, event::type::remove_segment, i);
        if (ang1 > ang2) {
            broom.insert(segment_ref { i });
        }
    }

    auto shoot = [&] (ncine::Vector2f through, segment seg) -> ncine::Vector2f {
        const auto d = ray_segment_test(origin, (through - origin), seg, true);
        return origin + (through - origin) * d;
    };

    // Process events in the queue
    int prev_min = broom.empty() ? -1 : broom.begin()->segment_id;
    ncine::Vector2f prev_pt;
    if (prev_min != -1) {
        prev_pt = shoot(origin + ncine::Vector2f(1.f, 0.f), segments[prev_min]);
    }
    while (!evts.empty()) {
        const auto evt = evts.top();
        evts.pop();

        ncine::Vector2f pt;
        if (evt.typ == event::type::add_segment) {
            broom.insert(segment_ref { evt.segment_id });
            pt = segments[evt.segment_id].first;
        } else {
            broom.erase(segment_ref { evt.segment_id });
            pt = segments[evt.segment_id].second;
        }

        int curr_min = broom.empty() ? -1 : broom.begin()->segment_id;

        if (prev_min != curr_min) {
            if (prev_min != -1) {
                // Produce the previous segment
                ret.push_back(segment {
                    prev_pt,
                    shoot(pt, segments[prev_min])
                });
            }

            if (curr_min != -1) {
                // Begin the next segment
                prev_pt = shoot(pt, segments[curr_min]);
            }
        }

        prev_min = curr_min;
    }

    if (prev_min != -1) {
        ret.push_back(segment {
            prev_pt,
            shoot(origin + ncine::Vector2f(1.f, 0.f), segments[prev_min])
        });
    }

    return ret;
}

}
