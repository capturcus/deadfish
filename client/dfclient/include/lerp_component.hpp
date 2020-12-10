#include <ncine/Sprite.h>

#include "../../../common/constants.hpp"
#include "../../../common/deadfish_generated.h"

class LerpComponent {
    ncine::Sprite* sprite;
	ncine::Vector2f prevPosition = {};
	ncine::Vector2f currPosition = {};
	float prevRotation = 0.f;
	float currRotation = 0.f;

public:
    inline LerpComponent() {}
    inline void bind(ncine::Sprite* aSprite) { sprite = aSprite; }
	inline void setupLerp(const float x, const float y, const float angle, bool firstUpdate) {
        prevPosition = currPosition;
        prevRotation = currRotation;

        currPosition.x = x * METERS2PIXELS;
        currPosition.y = -y * METERS2PIXELS;
        currRotation = -angle * 180.f / M_PI;

        if (firstUpdate) {
            prevPosition = currPosition;
            prevRotation = currRotation;
        }
    }

	inline void updateLerp(float subDelta) {
        float angleDelta = currRotation - prevRotation;
        if (angleDelta > M_PI) {
            angleDelta -= 2.f * M_PI;
        } else if (angleDelta < -M_PI) {
            angleDelta += 2.f * M_PI;
        }

        sprite->setPosition(prevPosition + (currPosition - prevPosition) * subDelta);
        sprite->setRotation(prevRotation + angleDelta * subDelta);
    }
};