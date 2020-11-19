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
	void ProcessDeathReport(const FlatBuffGenerated::DeathReport* deathReport);
	std::unique_ptr<ncine::TextNode> processMultikill(int multikillness);
	void endKillingSpree();
	void ProcessHighscoreUpdate(const FlatBuffGenerated::HighscoreUpdate* highscoreUpdate);
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

	typedef std::vector<std::unique_ptr<ncine::DrawableNode>> DrawableNodeVector;
	DrawableNodeVector nodes;
	DrawableNodeVector textNodes;
	std::unique_ptr<ncine::TextNode> killingSpreeText;
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
};

template<typename T, typename F>
void GameplayState::processMovable(std::map<uint16_t, T>& map, const FlatBuffGenerated::MovableComponent* comp,
	F createMovableFunc)
{
	auto it = map.find(comp->ID());
	if (it == map.end()) {
		// we see it for the first time
		T newMov = createMovableFunc();
		newMov.lerp.bind(static_cast<ncine::DrawableNode*>(newMov.sprite.get()));

		//fade in
		newMov.sprite->setAlpha(1);
		newMov.tween = CreateAlphaTransitionTween(newMov.sprite.get(), 1, 255, MOB_FADEIN_TIME);
		newMov.movableID = comp->ID();
		it = map.insert({comp->ID(), std::move(newMov)}).first;
	}
	T& mov = it->second;
	mov.lerp.setupLerp(comp->pos().x(), comp->pos().y(), comp->angle());
	mov.seen = true;
	if (mov.isAfterimage) {
		//re-fade in
		mov.tween = CreateAlphaTransitionTween(mov.sprite.get(), mov.sprite->alpha(), 255, MOB_FADEIN_TIME*(1 - mov.sprite->alpha()/255));
		mov.isAfterimage = false;
	}
}

template<typename T>
void GameplayState::deleteUnusedMovables(std::map<uint16_t, T>& map)
{
	std::vector<int> deletedIDs;
	for (auto& it : map) {
		auto& mov = it.second;
		if (!mov.seen) { // not seen
			if (!mov.isAfterimage) {	// not seen and not afterimage
				// fade out
				mov.tween = CreateAlphaTransitionTween(mov.sprite.get(), mov.sprite->alpha(), 0, MOB_FADEOUT_TIME*(mov.sprite->alpha()/255));
				mov.isAfterimage = true;
			} else if (mov.sprite->alpha() == 0) { // not seen and afterimage and alpha == 0
				deletedIDs.push_back(it.first);
			}
		}
	}
	for (auto id : deletedIDs)
		map.erase(id);
}
