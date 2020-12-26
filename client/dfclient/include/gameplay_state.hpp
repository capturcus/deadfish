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

#include "fov.hpp"
#include "game_state.hpp"
#include "lerp_component.hpp"
#include "../../../common/deadfish_generated.h"

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

struct Mob {
	std::unique_ptr<ncine::AnimatedSprite> sprite;
	LerpComponent lerp;
	bool seen;
	bool isAfterimage = false;
	FlatBuffGenerated::MobState state = FlatBuffGenerated::MobState_Walk;
	std::unique_ptr<ncine::Sprite> hoverMarker;
	std::unique_ptr<ncine::Sprite> relationMarker;
};

class Resources;
class TextCreator;

struct InkParticle {
	std::unique_ptr<ncine::DrawableNode> sprite;
	LerpComponent lerp;
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
	void CreateHidingSpotShowingTween(ncine::DrawableNode* hspot);
	void CreateHidingSpotHidingTween(ncine::DrawableNode* hspot);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewMobSprite(ncine::SceneNode* parent, uint16_t species);
	std::unique_ptr<ncine::AnimatedSprite> CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species, const std::string& spritesheet, uint16_t maxAnimations, Layers layer);
	nc::MeshSprite* CreateIndicator(float angle, float force, int indicatorNum, bool visible);
	void TryUseSkill(uint8_t skillPos);

	void updateRemainingText(uint64_t remainingFrames);
	void updateShadows();

	/** A subfunction of ProcessDeathReport, responsible for handling killing somebody.
	 */
	void processKill(const FlatBuffGenerated::DeathReport* deathReport);

	/** A subfunction of ProcessDeathReport, responsible for handling being killed.
	 */
	void processDeath(const FlatBuffGenerated::DeathReport* deathReport);

	/** A subfunction of ProcessDeathReport, responsible for handling reports of a kill 
	 * which doesn't involve the player.
	 */
	void processObituary(const FlatBuffGenerated::DeathReport* deathReport);

	/** A subfunction of processKill, responsible for managing the client's reaction to the player's multikill.
	 * @return A textnode with the adequate multikill text, to be included in the summary
	 */
	std::unique_ptr<ncine::TextNode> processMultikill(int multikillness);

	void endKillingSpree();


	friend class TextCreator;

	typedef std::vector<std::unique_ptr<ncine::DrawableNode>> DrawableNodeVector;
	DrawableNodeVector nodes;
	DrawableNodeVector textNodes;
	std::unique_ptr<ncine::TextNode> killingSpreeText;
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

	fov::mesh shadowMesh;
	std::unique_ptr<ncine::MeshSprite> shadowNode;

	std::vector<std::unique_ptr<nc::DrawableNode>> skillIcons;

	ncine::TimeStamp lastMessageReceivedTime;

	Resources& _resources;
};