#include <ncine/Sprite.h>

#include "../../../common/constants.hpp"
#include "../../../common/deadfish_generated.h"

class LerpComponent {
	ncine::DrawableNode* _sprite;
	ncine::Vector2f _prevPosition = {};
	ncine::Vector2f _currPosition = {};
	float _prevRotation = 0.f;
	float _currRotation = 0.f;
	bool _firstUpdate = true;

public:
	inline LerpComponent() {}
	inline void bind(ncine::DrawableNode* aSprite) { _sprite = aSprite; }
	inline void setupLerp(const float x, const float y, const float angle) {
		_prevPosition = _currPosition;
		_prevRotation = _currRotation;

		_currPosition.x = x * METERS2PIXELS;
		_currPosition.y = -y * METERS2PIXELS;
		_currRotation = -angle * 180.f / M_PI;

		if (_firstUpdate) {
			_prevPosition = _currPosition;
			_prevRotation = _currRotation;
			_firstUpdate = false;
		}
	}

	inline void updateLerp(float subDelta) {
		float angleDelta = _currRotation - _prevRotation;
		if (angleDelta > M_PI) {
			angleDelta -= 2.f * M_PI;
		} else if (angleDelta < -M_PI) {
			angleDelta += 2.f * M_PI;
		}

		_sprite->setPosition(_prevPosition + (_currPosition - _prevPosition) * subDelta);
		_sprite->setRotation(_prevRotation + angleDelta * subDelta);
	}
};