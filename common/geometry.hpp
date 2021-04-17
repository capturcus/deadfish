#pragma once

static float normalizeAngle(float angle) {
	while (angle < -180.f)
		angle += 360.f;
	while (angle > 180.f)
		angle -= 360.f;
	return angle;
}

template<typename Vectorlike>
inline float angleFromVector(const Vectorlike &v) {
	return (float) atan2(v.y, v.x);
}

template<typename Vectorlike>
inline Vectorlike rotateVector(const Vectorlike& v, float angle) {
	return Vectorlike(v.x*cos(angle) + v.y*(-sin(angle)), v.x*sin(angle) + v.y*cos(angle));
}
