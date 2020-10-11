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
const unsigned short DECORATION_LAYER = 8192;
const unsigned short MOBS_LAYER = 16384;
const unsigned short OBJECTS_LAYER = 20480;
const unsigned short HIDING_SPOTS_LAYER = 24576;

struct Mob {
	std::unique_ptr<ncine::AnimatedSprite> sprite;
	bool seen;
	FlatBuffGenerated::MobState state = FlatBuffGenerated::MobState_Walk;
	std::unique_ptr<ncine::Sprite> hoverMarker;
	std::unique_ptr<ncine::Sprite> relationMarker;

	ncine::Vector2f prevPosition = {};
	ncine::Vector2f currPosition = {};
	float prevRotation = 0.f;
	float currRotation = 0.f;

	void setupLocRot(const FlatBuffGenerated::Mob& msg, bool firstUpdate);
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
	void ProcessDeathReport(const FlatBuffGenerated::DeathReport* deathReport);
	void ProcessHighscoreUpdate(const FlatBuffGenerated::HighscoreUpdate* highscoreUpdate);
	void CreateHidingSpotShowingTween(ncine::DrawableNode* hspot);
	void CreateHidingSpotHidingTween(ncine::DrawableNode* hspot);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species);
	void ToggleHighscores();
	nc::MeshSprite* CreateIndicator(float angle, float force, int indicatorNum, bool visible);

	void updateRemainingText(uint64_t remainingFrames);

	typedef std::vector<std::unique_ptr<ncine::DrawableNode>> DrawableNodeVector;
	DrawableNodeVector nodes;
	std::map<std::string, DrawableNodeVector> hiding_spots;
	std::unique_ptr<ncine::SceneNode> cameraNode;
	std::map<uint16_t, Mob> mobs;
	ncine::Sprite* mySprite = nullptr;
	uint32_t lastNodeID = 0;
	bool showHighscores = false;
	bool showQuitDialog = false;
	bool gameEnded = false;
	std::vector<nc::DrawableNode*> indicators;
	ncine::TextNode* timeLeftNode = nullptr;
	std::string currentHidingSpot = "";

	ncine::TimeStamp lastMessageReceivedTime;

	Resources& _resources;
};
