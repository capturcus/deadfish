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

struct Resources {
	std::map<std::string, std::unique_ptr<ncine::Font>> fonts;
	std::map<std::string, std::unique_ptr<ncine::Texture>> textures;

	std::map<std::string, std::unique_ptr<ncine::AudioBuffer>> _sounds;
	std::unique_ptr<ncine::AudioBuffer> _killSoundBuffer;
	std::unique_ptr<ncine::AudioBufferPlayer> _killSound;
	std::unique_ptr<ncine::AudioBufferPlayer> _deathSound;

	std::vector<tweeny::tween<int>> _tweens;
	std::unordered_map<uint16_t, tweeny::tween<int>> _mobTweens;

	void UpdateTweens();

	/** Plays default kill sound
	 * @param gain volume (1.0f is 100%)
	*/
	void playKillSound(float gain = 1.0f);

	/** Plays random death sound
	*/
	void playRandomDeathSound();
};

#endif
