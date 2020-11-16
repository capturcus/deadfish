#include "resources.hpp"

void Resources::UpdateTweens()
{
    for (int i = _tweens.size() - 1; i >= 0; i--) {
		_tweens[i].step(1);
		if (_tweens[i].progress() == 1.f)
			_tweens.erase(_tweens.begin() + i);
	}
	for (auto& tween : _mobTweens) {
		tween.second.step(1);
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
