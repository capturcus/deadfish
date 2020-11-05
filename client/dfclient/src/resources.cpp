#include "resources.hpp"

void Resources::UpdateTweens()
{
    for (int i = _intTweens.size() - 1; i >= 0; i--) {
		_intTweens[i].step(1);
		if (_intTweens[i].progress() == 1.f)
			_intTweens.erase(_intTweens.begin() + i);
	}
	for (auto& tween : _mobTweens) {
		tween.second.step(1);
	}
	for (auto it = _floatTweens.begin(); it != _floatTweens.end(); ++it) {
		it->step(1);
		if (it->progress() == 1.f) {
			_floatTweens.erase(it);
			--it;
		}
	}
}

void Resources::playKillSound(float gain) {
	_killSound->setGain(gain);
	_killSound->play();
}

void Resources::playRandomDeathSound() {
	auto randIndex = rand()%_sounds.size();
	std::map<std::string, std::unique_ptr<ncine::AudioBuffer>>::iterator it = _sounds.begin();
	for(auto i=0; i<randIndex; ++i) ++it;
	_deathSound = std::make_unique<ncine::AudioBufferPlayer>(it->second.get());
	_deathSound->play();
}
