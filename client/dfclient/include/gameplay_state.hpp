#pragma once

#include <vector>
#include <memory>
#include <map>

#include <ncine/MeshSprite.h>
#include <ncine/AnimatedSprite.h>
#include <ncine/TextNode.h>
#include <ncine/TimeStamp.h>

namespace nc = ncine;

#include "tweeny.h"

#include "../../../common/constants.hpp"
#include "../../../common/deadfish_generated.h"
#include "../../../common/types.hpp"
#include "fov.hpp"
#include "game_state.hpp"
#include "lerp_component.hpp"
<<<<<<< HEAD
#include "../../../common/deadfish_generated.h"
#include "../../../common/types.hpp"

const int MOB_FADEIN_TIME = 5;
const int MOB_FADEOUT_TIME = 7;

enum class Layers {
	TILE = 0,
	DECORATION,
	INDICATOR,
	MOB_MANIPULATORS,
	MOBS,
	SHADOW,
	OBJECTS,
	INK_PARTICLES,
	HIDING_SPOTS,
	SKILLS,
	TEXT_OUTLINES,
	ADDITIONAL_TEXT,
	TEXT
};
=======
#include "death_report_processor.hpp"
>>>>>>> 11a59ab (death report refactor (DeathReportProcessor))

tweeny::tween<int>
CreateAlphaTransitionTween(ncine::DrawableNode* sprite, int from, int to, int during);

struct Movable {
	LerpComponent lerp;
	uint16_t movableID;
	bool seen;
	bool isAfterimage = false;
	std::unique_ptr<ncine::Sprite> sprite;
	tweeny::tween<int> tween;
};

struct Mob : public Movable {
	FlatBuffGenerated::MobState state = FlatBuffGenerated::MobState_Walk;
	std::unique_ptr<ncine::Sprite> hoverMarker;
	std::unique_ptr<ncine::Sprite> relationMarker;
};

class Resources;
class TextCreator;

struct InkParticle : public Movable {
};

struct Manipulator : public Movable {
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
	void CreateHidingSpotShowingTween(ncine::DrawableNode* hspot);
	void CreateHidingSpotHidingTween(ncine::DrawableNode* hspot);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewMobSprite(ncine::SceneNode* parent, uint16_t species);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species, const std::string& spritesheet, uint16_t maxAnimations, Layers layer);
	nc::MeshSprite* CreateIndicator(float angle, float force, int indicatorNum, bool visible);
	void TryUseSkill(uint8_t skillPos);

	void updateRemainingText(uint64_t remainingFrames);
	void updateShadows();
	void updateHovers(ncine::Vector2f mouseCoords, float radiusSquared);

	template<typename T, typename F>
	void processMovable(std::map<uint16_t, T>& map, const FlatBuffGenerated::MovableComponent* comp,
		F createMovableFunc);
	template<typename T>
	void deleteUnusedMovables(std::map<uint16_t, T>& map);

	std::map<uint16_t, Manipulator> manipulators;
	std::map<uint16_t, Mob> mobs;
	std::map<uint16_t, InkParticle> inkParticles;


	friend class TextCreator;

	DrawableNodeVector nodes;
	DrawableNodeVector textNodes;
	std::map<std::string, DrawableNodeVector> hiding_spots;
	std::unique_ptr<ncine::SceneNode> cameraNode;
	ncine::Sprite* mySprite = nullptr;
	uint32_t lastNodeID = 0;
	bool showHighscores = false;
	bool showQuitDialog = false;
	bool gameEnded = false;
	std::vector<nc::DrawableNode*> indicators;
	ncine::TextNode* timeLeftNode = nullptr;
	std::string currentHidingSpot = "";

	fov::mesh shadowMesh;
	std::unique_ptr<ncine::MeshSprite> shadowNode;

	std::vector<std::unique_ptr<nc::DrawableNode>> skillIcons;

	ncine::TimeStamp lastMessageReceivedTime;

	Resources& _resources;

	DeathReportProcessor _deathReportProcessor;
};
