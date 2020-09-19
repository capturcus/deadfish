#pragma once

#include <map>
#include <memory>

#include <ncine/TextNode.h>
#include <ncine/AudioBuffer.h>
#include <ncine/AudioBufferPlayer.h>

struct Resources {
	std::map<std::string, std::unique_ptr<ncine::Font>> fonts;
	std::map<std::string, std::unique_ptr<ncine::Texture>> textures;

	std::unique_ptr<ncine::AudioBuffer> _wilhelmAudioBuffer;
	std::unique_ptr<ncine::AudioBufferPlayer> _wilhelmSound;
};
