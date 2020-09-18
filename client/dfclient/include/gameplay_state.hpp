#pragma once

#include <vector>
#include <memory>
#include <map>

#include <ncine/MeshSprite.h>
#include <ncine/Sprite.h>
#include <ncine/TextNode.h>

namespace nc = ncine;

#include "tweeny.h"

#include "game_state.hpp"
#include "../../../common/deadfish_generated.h"

const float PIXELS2METERS = 0.01f;
const float METERS2PIXELS = 100.f;
const int MOBS_LAYER = 16384;

struct Mob {
	std::unique_ptr<ncine::AnimatedSprite> sprite;
	bool seen;
	DeadFish::MobState state = DeadFish::MobState_Walk;
	std::unique_ptr<ncine::Sprite> hoverMarker;
	std::unique_ptr<ncine::Sprite> relationMarker;
};

struct GameplayState
	: public GameState
{
	using GameState::GameState;
	
	void Create() override;
	void Update() override;
	void CleanUp() override;

	void OnMessage(const std::string& data);
	void OnMouseButtonPressed(const ncine::MouseEvent &event) override;
	void OnKeyPressed(const ncine::KeyboardEvent &event) override;
	void OnKeyReleased(const ncine::KeyboardEvent &event) override;
	void LoadLevel();
	void ProcessDeathReport(const DeadFish::DeathReport* deathReport);
	void ProcessHighscoreUpdate(const DeadFish::HighscoreUpdate* highscoreUpdate);
	void CreateTextTween(ncine::TextNode* textPtr);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species);
	void ToggleHighscores();
	nc::MeshSprite* CreateIndicator(float angle, float force, int indicatorNum, bool visible);

	std::vector<std::unique_ptr<ncine::DrawableNode>> nodes;
	std::unique_ptr<ncine::SceneNode> cameraNode;
	std::map<uint16_t, Mob> mobs;
	ncine::Sprite* mySprite = nullptr;
	uint32_t lastNodeID = 0;
	std::vector<tweeny::tween<int>> tweens;
	bool showHighscores = false;
	std::vector<nc::DrawableNode*> indicators;
};