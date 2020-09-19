#pragma once

#include <vector>
#include <memory>
#include <map>

#include <ncine/MeshSprite.h>
#include <ncine/Sprite.h>
#include <ncine/TextNode.h>
#include <ncine/TimeStamp.h>

namespace ncine {
	class AnimatedSprite;
}

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

	ncine::Vector2f prevPosition = {};
	ncine::Vector2f currPosition = {};
	float prevRotation = 0.f;
	float currRotation = 0.f;

	void setupLocRot(const DeadFish::Mob& msg);
	void updateLocRot(float subDelta);
};

class Resources;

struct GameplayState
	: public GameState
{
	GameplayState(Resources&);
	~GameplayState() override;
	
	StateType Update(Messages) override;

	void OnMouseButtonPressed(const ncine::MouseEvent &event) override;
	void OnKeyPressed(const ncine::KeyboardEvent &event) override;
	void OnKeyReleased(const ncine::KeyboardEvent &event) override;

private:
	void OnMessage(const std::string& data);

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

	ncine::TimeStamp lastMessageReceivedTime;

	Resources& _resources;
};
