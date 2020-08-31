#include <iostream>
#include <functional>
#include <complex>

#include <ncine/Application.h>
#include <ncine/Colorf.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>
#include <ncine/AnimatedSprite.h>

#include "../../../common/constants.hpp"

#include "gameplay_state.hpp"
#include "game_data.hpp"
#include "fb_util.hpp"
#include "state_manager.hpp"
#include "util.hpp"

const float ANIMATION_FPS = 20.f;
const int FISH_FRAME_WIDTH = 120;
const int FISH_FRAME_HEIGHT = 120;
const int IMGS_PER_ROW = 10;
const int IMGS_PER_SPECIES = 80;

enum FISH_ANIMATIONS {
	WALK = 0,
	RUN,
	ATTACK,
	MAX
};

int ANIM_LENGTHS[] = {
	[FISH_ANIMATIONS::WALK] = 20,
	[FISH_ANIMATIONS::RUN] = 20,
	[FISH_ANIMATIONS::ATTACK] = 40,
};

ncine::Vector2i spriteCoords(int spriteNum) {
	int row = spriteNum / IMGS_PER_ROW;
	int col = spriteNum % IMGS_PER_ROW;
	return {col * FISH_FRAME_WIDTH, row * FISH_FRAME_HEIGHT};
}

std::unique_ptr<ncine::AnimatedSprite> GameplayState::CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species) {
	std::unique_ptr<ncine::AnimatedSprite> ret = std::make_unique<ncine::AnimatedSprite>(parent, manager.textures["fish.png"].get());
	int currentImg = species * IMGS_PER_SPECIES;
	for (int animNumber = 0; animNumber < FISH_ANIMATIONS::MAX; animNumber++) {
		nctl::UniquePtr<ncine::RectAnimation> animation =
		nctl::makeUnique<ncine::RectAnimation>(1./ANIMATION_FPS,
			ncine::RectAnimation::LoopMode::ENABLED,
			ncine::RectAnimation::RewindMode::FROM_START);
		for (int j = 0; j < ANIM_LENGTHS[animNumber]; j++) {
			auto coords = spriteCoords(currentImg);
			animation->addRect(coords.x, coords.y, FISH_FRAME_WIDTH, FISH_FRAME_HEIGHT);
			currentImg++;
		}
		ret->addAnimation(nctl::move(animation));
	}

	ret->setAnimationIndex(FISH_ANIMATIONS::WALK);
	ret->setFrame(0);
	ret->setPaused(false);
	ret->setLayer(MOBS_LAYER);
	return std::move(ret);
}

void GameplayState::LoadLevel() {
	auto level = FBUtilGetServerEvent(gameData.levelData, Level);

	for (int i = 0; i < level->stones()->size(); i++) {
		auto stone = level->stones()->Get(i);
		auto stoneSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), manager.textures["stone.png"].get(),
			stone->pos()->x() * METERS2PIXELS, -stone->pos()->y() * METERS2PIXELS);
		this->nodes.push_back(std::move(stoneSprite));
	}

	for (int i = 0; i < level->bushes()->size(); i++) {
		auto bush = level->bushes()->Get(i);
		auto bushSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), manager.textures["bush.png"].get(),
			bush->pos()->x() * METERS2PIXELS, -bush->pos()->y() * METERS2PIXELS);
		this->nodes.push_back(std::move(bushSprite));
	}
}

void GameplayState::ProcessDeathReport(const DeadFish::DeathReport* deathReport) {
	auto& rootNode = ncine::theApplication().rootNode();
	auto text = std::make_unique<ncine::TextNode>(&rootNode, this->manager.fonts["comic"].get());
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	if (deathReport->killer() == gameData.myPlayerID) {
		// i killed someone, yay
		text->setString(("you killed " + gameData.players[deathReport->killed()].name + "!").c_str());
		text->setColor(0, 255, 0, 255);
		text->setPosition(screenWidth * 0.5f, screenHeight * 0.75f);
		text->setScale(3.0f);
		auto textPtr = text.get();
		auto tween = tweeny::from(255)
			.to(255).during(60)
			.to(0).during(60).onStep(
			[textPtr] (tweeny::tween<int>& t, int v) -> bool {
				textPtr->setColor(0, 255, 0, v);
				return false;
			}
		);
		this->tweens.push_back(std::move(tween));
	} else if (deathReport->killed() == gameData.myPlayerID) {
		// i died :c
		text->setString(("you have been killed by " + gameData.players[deathReport->killer()].name).c_str());
		text->setColor(255, 0, 0, 255);
		text->setPosition(screenWidth * 0.5f, screenHeight * 0.5f);
		text->setScale(2.0f);
	} else {
		auto killer = gameData.players[deathReport->killer()].name;
		auto killed = gameData.players[deathReport->killed()].name;
		text->setString((killer + " killed " + killed).c_str());
		text->setScale(0.5f);
		text->setPosition(screenWidth * 0.8f, screenHeight * 0.8f);
		text->setColor(0, 0, 0, 255);
	}
	this->nodes.push_back(std::move(text));
}

void GameplayState::OnMessage(const std::string& data) {
	auto deathReport = FBUtilGetServerEvent(data, DeathReport);
	if (deathReport) {
		this->ProcessDeathReport(deathReport);
		return;
	}

	auto worldState = FBUtilGetServerEvent(data, WorldState);
	if (!worldState)
		return;

	// reset seen status of mobs
	for (auto& p : this->mobs)
		p.second.seen = false;

	for (int i = 0; i < worldState->mobs()->size(); i++) {
		auto mobData = worldState->mobs()->Get(i);
		auto mobItr = this->mobs.find(mobData->mobID());
		if (mobItr == this->mobs.end()) {
			// this is the first time we see this mob, create it
			Mob newMob;
			newMob.sprite = CreateNewAnimSprite(this->cameraNode.get(), mobData->species());
			this->mobs[mobData->mobID()] = std::move(newMob);
			mobItr = this->mobs.find(mobData->mobID());
		}
		Mob& mob = mobItr->second;
		mob.sprite->setPosition(mobData->pos()->x() * METERS2PIXELS, -mobData->pos()->y() * METERS2PIXELS);
		mob.sprite->setRotation(-mobData->angle() * 180 / M_PI);
		mob.seen = true;
		if (mobData->state() != mob.state) {
			mob.state = mobData->state();
			mob.sprite->setAnimationIndex(mobData->state());
			mob.sprite->setFrame(0);
			mob.sprite->setPaused(false);
		}

		// if it's us then save sprite
		if (mobItr->first == gameData.myMobID) {
			this->mySprite = mob.sprite.get();
		}

		if (mobData->relation() == DeadFish::PlayerRelation_None) {
			mob.relationMarker.reset(nullptr);
		} else if (mobData->relation() == DeadFish::PlayerRelation_Close) {
			mob.relationMarker = std::make_unique<ncine::Sprite>(mob.sprite.get(), manager.textures["bluecircle.png"].get());
			mob.relationMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
		} else if (mobData->relation() == DeadFish::PlayerRelation_Targeted) {
			mob.relationMarker = std::make_unique<ncine::Sprite>(mob.sprite.get(), manager.textures["redcircle.png"].get());
			mob.relationMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
		}
	}
	std::vector<int> deletedIDs;
	for (auto& mob : this->mobs) {
		if (!mob.second.seen)
			deletedIDs.push_back(mob.first);
	}
	for (auto id : deletedIDs) {
		this->mobs.erase(id);
		if (id == gameData.myMobID)
			this->mySprite = nullptr;
	}
}

void GameplayState::Create() {
	std::cout << "entered gameplay state\n";
	ncine::theApplication().gfxDevice().setClearColor(ncine::Colorf(1, 1, 1, 1));
	auto& rootNode = ncine::theApplication().rootNode();
	this->cameraNode = std::make_unique<ncine::SceneNode>(&rootNode);
	this->LoadLevel();
	gameData.socket->onMessage = std::bind(&GameplayState::OnMessage, this, std::placeholders::_1);
}

void GameplayState::Update() {
	for (int i = this->tweens.size() - 1; i >= 0; i--) {
		this->tweens[i].step(1);
		if (this->tweens[i].progress() == 1.f)
			this->tweens.erase(this->tweens.begin() + i);
	}
	if (this->mySprite == nullptr)
		return;
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	auto &mouseState = ncine::theApplication().inputManager().mouseState();
	auto myMobPosition = -this->mySprite->position() +
		ncine::Vector2f(3*screenWidth/4 - mouseState.x/2,
						screenHeight/4 + mouseState.y/2);
	this->cameraNode->setPosition(myMobPosition);

	ncine::Vector2f mouseCoords;
	mouseCoords.x = this->mySprite->position().x + (mouseState.x - screenWidth / 2) * 1.5f;
	mouseCoords.y = this->mySprite->position().y + -(mouseState.y - screenHeight / 2) * 1.5f;
	float radiusSquared = this->mySprite->size().x/2;
	radiusSquared = radiusSquared*radiusSquared;

	// hovering
	int smallestNorm = INT32_MAX;
	Mob* closestMob = nullptr;
	for (auto& mob : this->mobs) {
		if (mob.second.sprite.get() == this->mySprite)
			continue;

		mob.second.hoverMarker.reset(nullptr);

		int norm = deadfish::norm(mob.second.sprite->position(), mouseCoords);
		if (norm < smallestNorm) {
			smallestNorm = norm;
			closestMob = &mob.second;
		}
	}
	if (closestMob && smallestNorm < radiusSquared) {
		closestMob->hoverMarker = std::make_unique<ncine::Sprite>(closestMob->sprite.get(), manager.textures["graycircle.png"].get());
		closestMob->hoverMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
	}
}

void GameplayState::CleanUp() {

}

void GameplayState::OnMouseButtonPressed(const ncine::MouseEvent &event) {
	if (this->mySprite == nullptr)
		return;
	if (event.isLeftButton()) {
		// move
		const float screenWidth = ncine::theApplication().width();
		const float screenHeight = ncine::theApplication().height();
		const float serverX = (this->mySprite->position().x + (event.x - screenWidth / 2) * 1.5f) * PIXELS2METERS;
		const float serverY = -(this->mySprite->position().y + (event.y - screenHeight / 2) * 1.5f) * PIXELS2METERS;
		flatbuffers::FlatBufferBuilder builder;
		auto pos = DeadFish::Vec2(serverX, serverY);
		auto cmdMove = DeadFish::CreateCommandMove(builder, &pos);
		auto message = DeadFish::CreateClientMessage(builder, DeadFish::ClientMessageUnion_CommandMove, cmdMove.Union());
		builder.Finish(message);
		SendData(builder);
	}
	if (event.isRightButton()) {
		// kill
		for (auto& mob : this->mobs) {
			if (mob.second.hoverMarker.get()) {
				// if it is hovered kill it
				flatbuffers::FlatBufferBuilder builder;
				auto cmdKill = DeadFish::CreateCommandKill(builder, mob.first);
				auto message = DeadFish::CreateClientMessage(builder, DeadFish::ClientMessageUnion_CommandKill, cmdKill.Union());
				builder.Finish(message);
				SendData(builder);
				break;
			}
		}
	}
}

void SendCommandRun(bool run) {
	flatbuffers::FlatBufferBuilder builder;
	auto cmdRun = DeadFish::CreateCommandRun(builder, run);
	auto message = DeadFish::CreateClientMessage(builder, DeadFish::ClientMessageUnion_CommandRun, cmdRun.Union());
	builder.Finish(message);
	SendData(builder);
}

void GameplayState::OnKeyPressed(const ncine::KeyboardEvent &event) {
	if (event.sym == ncine::KeySym::Q)
		SendCommandRun(true);
}

void GameplayState::OnKeyReleased(const ncine::KeyboardEvent &event) {
	if (event.sym == ncine::KeySym::Q)
		SendCommandRun(false);
}