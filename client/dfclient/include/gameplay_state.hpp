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
enum class Layers {
	TILE = 0,
	DECORATION,
	INDICATOR,
	MOB_MANIPULATORS,
	MOBS,
	OBJECTS,
	INK_PARTICLES,
	HIDING_SPOTS,
	SKILLS,
	ADDITIONAL_TEXT,
	TEXT
};

struct Mob {
	std::unique_ptr<ncine::AnimatedSprite> sprite;
	bool seen;
	bool isAfterimage = false;
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
class TextCreator;

struct InkParticle {
	std::unique_ptr<ncine::DrawableNode> sprite;
	bool seen = false;
};

struct GameplayState
	: public GameState
{
	GameplayState(Resources&);
	~GameplayState() override;
	
	StateType Update(Messages) override;

	void OnMouseButtonPressed(const ncine::MouseEvent &event) override;
	void OnKeyPressed(const ncine::KeyboardEvent &event) override;
	void OnKeyReleased(const ncine::KeyboardEvent &event) override;

	void ProcessDeathReport(const void* deathReport);
	void ProcessHighscoreUpdate(const void* highscoreUpdate);
	void ProcessSimpleServerEvent(const void* simpleServerEvent);
	void ProcessWorldState(const void* worldState);
	void ProcessSkillBarUpdate(const void* worldState);

private:
	void OnMessage(const std::string& data);

	void LoadLevel();
	void ProcessDeathReport(const FlatBuffGenerated::DeathReport* deathReport);
	std::unique_ptr<ncine::TextNode> processMultikill(int multikillness);
	void ProcessHighscoreUpdate(const FlatBuffGenerated::HighscoreUpdate* highscoreUpdate);
	void CreateHidingSpotShowingTween(ncine::DrawableNode* hspot);
	void CreateHidingSpotHidingTween(ncine::DrawableNode* hspot);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewMobSprite(ncine::SceneNode* parent, uint16_t species);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species, const std::string& spritesheet, uint16_t maxAnimations, Layers layer);
	nc::MeshSprite* CreateIndicator(float angle, float force, int indicatorNum, bool visible);
	void TryUseSkill(uint8_t skillPos);

	void updateRemainingText(uint64_t remainingFrames);

	friend class TextCreator;

	typedef std::vector<std::unique_ptr<ncine::DrawableNode>> DrawableNodeVector;
	DrawableNodeVector nodes;
	DrawableNodeVector textNodes;
	std::map<std::string, DrawableNodeVector> hiding_spots;
	std::unique_ptr<ncine::SceneNode> cameraNode;
	std::map<uint16_t, Mob> mobs;
	std::map<uint16_t, InkParticle> inkParticles;
	ncine::Sprite* mySprite = nullptr;
	uint32_t lastNodeID = 0;
	bool showHighscores = false;
	bool showQuitDialog = false;
	bool gameEnded = false;
	std::vector<nc::DrawableNode*> indicators;
	std::vector<std::unique_ptr<ncine::Sprite>> manipulators;
	ncine::TextNode* timeLeftNode = nullptr;
	std::string currentHidingSpot = "";

	std::vector<std::unique_ptr<ncine::DrawableNode>> skillIcons;

	ncine::TimeStamp lastMessageReceivedTime;

	Resources& _resources;
};
