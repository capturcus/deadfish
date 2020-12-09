#pragma once
#ifndef __RESOURCES_HPP__
#define __RESOURCES_HPP__

#include <map>
#include <unordered_map>
#include <string>
#include <memory>

#include <ncine/TextNode.h>
#include <ncine/AudioBuffer.h>
#include <ncine/AudioBufferPlayer.h>

#include "tweeny.h"

static const char* TEXTURES_PATH = "textures";
static const char* LEVELS_PATH = "../levels";
static const char* SOUNDS_PATH = "sounds";

enum class SoundType {
	KILL,
	DEATH,
	GOLDFISH
};

struct Resources {
	std::map<std::string, std::unique_ptr<ncine::Font>> fonts;
	std::map<std::string, std::unique_ptr<ncine::Texture>> textures;

	std::map<std::string, std::unique_ptr<ncine::AudioBuffer>> _sounds;
	std::unique_ptr<ncine::AudioBuffer> _killSoundBuffer;
	std::unique_ptr<ncine::AudioBuffer> _goldfishSoundBuffer;
	std::unique_ptr<ncine::AudioBufferPlayer> _killSound;
	std::unique_ptr<ncine::AudioBufferPlayer> _deathSound;
	std::unique_ptr<ncine::AudioBufferPlayer> _goldfishSound;

	std::vector<tweeny::tween<int>> _intTweens;
	std::unordered_map<uint16_t, tweeny::tween<int>> _mobTweens;
	std::vector<tweeny::tween<float>> _floatTweens;
	std::optional<tweeny::tween<int>> _killingSpreeTween;

	void UpdateTweens();

	/** Plays a sound
	 * @param sound which sound type to play
	 * @param gain volume (1.0f is 100%)
	*/
	void playSound(SoundType soundType, float gain = 1.0f);
};

#endif
