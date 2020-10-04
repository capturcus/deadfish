#pragma once
#ifndef __RESOURCES_HPP__
#define __RESOURCES_HPP__

#include <map>
#include <memory>

#include <ncine/TextNode.h>
#include <ncine/AudioBuffer.h>
#include <ncine/AudioBufferPlayer.h>

#include "tweeny.h"

const char* TEXTURES_PATH;
const char* LEVELS_PATH;

struct Resources {
	std::map<std::string, std::unique_ptr<ncine::Font>> fonts;
	std::map<std::string, std::unique_ptr<ncine::Texture>> textures;

	std::unique_ptr<ncine::AudioBuffer> _wilhelmAudioBuffer;
	std::unique_ptr<ncine::AudioBufferPlayer> _wilhelmSound;

	std::vector<tweeny::tween<int>> _tweens;

	void UpdateTweens();
};

#endif
