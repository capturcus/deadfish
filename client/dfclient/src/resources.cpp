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
	if (_killingSpreeTween.has_value()){
		_killingSpreeTween->step(1);
		if (_killingSpreeTween->progress() == 1.f) {
			_killingSpreeTween->backward();
		}
		if (_killingSpreeTween->progress() == 0.f) {
			_killingSpreeTween->forward();
		}
	}
}

void Resources::playSound(SoundType soundType, float gain) {
	ncine::AudioBufferPlayer* soundPlayer;
	switch (soundType)
	{
	case SoundType::KILL:
		soundPlayer = _killSound.get();
		break;
	case SoundType::GOLDFISH:
		soundPlayer = _goldfishSound.get();
		break;
	case SoundType::DEATH:
		auto randIndex = rand()%_sounds.size();
		auto it = _sounds.begin();
		for (auto i=0; i<randIndex; ++i) ++it;
		_deathSound = std::make_unique<ncine::AudioBufferPlayer>(it->second.get());
		break;
	}
	soundPlayer->setGain(gain);
	soundPlayer->play();
}

