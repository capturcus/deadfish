#include "resources.hpp"

const char* TEXTURES_PATH = "textures";
const char* LEVELS_PATH = "../levels";

void Resources::UpdateTweens()
{
    for (int i = _tweens.size() - 1; i >= 0; i--) {
		_tweens[i].step(1);
		if (_tweens[i].progress() == 1.f)
			_tweens.erase(_tweens.begin() + i);
	}
}