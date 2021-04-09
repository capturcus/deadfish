#pragma once

#include <memory>
#include <vector>

#include "game_state.hpp"
#include "ncine/TextNode.h"
#include "ncine/Sprite.h"
#include "ncine/MeshSprite.h"

#include "tweeny.h"

class Resources;

struct LoadingState
	: public GameState
{
public:
	LoadingState(Resources& r);
	~LoadingState() override;
	
	StateType Update(Messages) override;

private:
	long _total = 0;
	long _current = 0;
	std::vector<std::string> _textureFilenames;
	std::vector<std::string> _soundFilenames;
	Resources& _resources;
	std::unique_ptr<ncine::TextNode> _loadingText;
	std::unique_ptr<ncine::Sprite> _fillupSprite;
	std::unique_ptr<ncine::Sprite> _outline;
	std::unique_ptr<ncine::MeshSprite> _curtain;
	tweeny::tween<int>* _fadeoutTween = nullptr;
};
