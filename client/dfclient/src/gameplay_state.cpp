#include <complex>
#include <functional>
#include <iostream>

#include <ncine/AnimatedSprite.h>
#include <ncine/Application.h>
#include <ncine/Colorf.h>
#include <ncine/imgui.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>

#include "../../../common/constants.hpp"

#include "fb_util.hpp"
#include "game_data.hpp"
#include "gameplay_state.hpp"
#include "resources.hpp"
#include "util.hpp"

const float ANIMATION_FPS = 20.f;
const int FISH_FRAME_WIDTH = 120;
const int FISH_FRAME_HEIGHT = 120;
const int IMGS_PER_ROW = 10;
const int IMGS_PER_SPECIES = 80;

const int MAX_HIDING_SPOT_OPACITY = 255;
const int MIN_HIDING_SPOT_OPACITY = 128;

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
	std::unique_ptr<ncine::AnimatedSprite> ret = std::make_unique<ncine::AnimatedSprite>(parent, _resources.textures["fish.png"].get());
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
		auto stoneSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures["stone.png"].get(),
			stone->pos()->x() * METERS2PIXELS, -stone->pos()->y() * METERS2PIXELS);
		this->nodes.push_back(std::move(stoneSprite));
	}

	for (int i = 0; i < level->hidingspots()->size(); i++) {
		auto hspot = level->hidingspots()->Get(i);
		auto hspotSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures["bush.png"].get(),
			hspot->pos()->x() * METERS2PIXELS, -hspot->pos()->y() * METERS2PIXELS);
		hspotSprite->setLayer(HIDING_SPOTS_LAYER);
		this->hiding_spots.push_back(std::move(hspotSprite));
	}
}


// this whole thing should probably be refactored
void GameplayState::ProcessDeathReport(const DeadFish::DeathReport* deathReport) {
	auto& rootNode = ncine::theApplication().rootNode();
	auto text = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	if (deathReport->killer() == gameData.myPlayerID) {
		// i killed someone
		std::string killedName;
		if (deathReport->killed() == (uint16_t)-1) {
			// it was an npc
			killedName = "a civilian";
			text->setColor(0, 0, 0, 255);
		} else {
			// it was a player
			text->setColor(0, 255, 0, 255);
			killedName = gameData.players[deathReport->killed()].name + "!";
		}
		text->setString(("you killed " + killedName).c_str());
		text->setPosition(screenWidth * 0.5f, screenHeight * 0.75f);
		text->setScale(3.0f);
		_resources._tweens.push_back(CreateTextTween(text.get()));
		
	} else if (deathReport->killed() == gameData.myPlayerID) {
		// i died :c
		_resources._wilhelmSound->play();

		text->setString(("you have been killed by " + gameData.players[deathReport->killer()].name).c_str());
		text->setColor(255, 0, 0, 255);
		text->setPosition(screenWidth * 0.5f, screenHeight * 0.5f);
		text->setScale(2.0f);
		_resources._tweens.push_back(CreateTextTween(text.get()));
	} else {
		auto killer = gameData.players[deathReport->killer()].name;
		auto killed = gameData.players[deathReport->killed()].name;
		text->setString((killer + " killed " + killed).c_str());
		text->setScale(0.5f);
		text->setPosition(screenWidth * 0.8f, screenHeight * 0.8f);
		text->setColor(0, 0, 0, 255);
		_resources._tweens.push_back(CreateTextTween(text.get()));
	}
	this->nodes.push_back(std::move(text));
}

void GameplayState::ProcessHighscoreUpdate(const DeadFish::HighscoreUpdate* highscoreUpdate) {
	for (int i = 0; i < highscoreUpdate->players()->size(); i++) {
		auto highscoreEntry = highscoreUpdate->players()->Get(i);
		// this is ugly, i know
		for (auto& p : gameData.players) {
			if (p.playerID == highscoreEntry->playerID())
				p.score = highscoreEntry->playerPoints();
		}
	}
}

const float INDICATOR_WIDTH = 12.f;
const float INDICATOR_OPACITY = 0.3f;
const float INDICATOR_OFFSET = 80.f;

/**
 * @param angle angle to target in radians
 * @param force force of the indicator from 0.0 to 1.0
 * */
nc::MeshSprite* GameplayState::CreateIndicator(float angle, float force, int indicatorNum, bool visible) {
	auto arc = createArc(*this->mySprite, _resources.textures["pixel.png"].get(), 0, 0,
		INDICATOR_OFFSET + indicatorNum * INDICATOR_WIDTH,
		INDICATOR_OFFSET + (indicatorNum+1) * INDICATOR_WIDTH, force * 360.f);
	arc->setRotation(-this->mySprite->rotation() - angle * TO_DEGREES - force * 180.f + 180.f);
	if (visible)
		arc->setColor(255, 255, 255, INDICATOR_OPACITY * 255);
	else
		arc->setColor(0, 0, 0, INDICATOR_OPACITY * 255);
	return arc;
}

void GameplayState::OnMessage(const std::string& data) {
	lastMessageReceivedTime = ncine::TimeStamp::now();

	auto highscoreUpdate = FBUtilGetServerEvent(data, HighscoreUpdate);
	if (highscoreUpdate) {
		this->ProcessHighscoreUpdate(highscoreUpdate);
		return;
	}

	auto deathReport = FBUtilGetServerEvent(data, DeathReport);
	if (deathReport) {
		this->ProcessDeathReport(deathReport);
		return;
	}

	auto simpleServerEvent = FBUtilGetServerEvent(data, SimpleServerEvent);
	if (simpleServerEvent) {
		if (simpleServerEvent->type() == DeadFish::SimpleServerEventType_GameEnded) {
			gameEnded = true;
			updateRemainingText(0);
		}

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
		mob.setupLocRot(*mobData);
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
			mob.relationMarker = std::make_unique<ncine::Sprite>(mob.sprite.get(), _resources.textures["bluecircle.png"].get());
			mob.relationMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
		} else if (mobData->relation() == DeadFish::PlayerRelation_Targeted) {
			mob.relationMarker = std::make_unique<ncine::Sprite>(mob.sprite.get(), _resources.textures["redcircle.png"].get());
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

	if (this->mySprite == nullptr)
		return;

	// draw indicators

	for (auto ind : this->indicators) {
		this->mySprite->removeChildNode(ind);
	}

	this->indicators.resize(0);
	this->indicators.resize(gameData.players.size());

	for (int i = 0; i < worldState->indicators()->size(); i++) {
		auto ind = worldState->indicators()->Get(i);
		if (ind->force() * 360.f < 1.f)
			continue;
		auto indArc = CreateIndicator(ind->angle(), ind->force(), i, ind->visible());
		this->indicators[i] = indArc;
	}

	// draw remaining time
	updateRemainingText(worldState->stepsRemaining());
}

void Mob::setupLocRot(const DeadFish::Mob& msg) {
	prevPosition = currPosition;
	prevRotation = currRotation;

	currPosition.x = msg.pos()->x() * METERS2PIXELS;
	currPosition.y = -msg.pos()->y() * METERS2PIXELS;
	currRotation = -msg.angle() * 180.f / M_PI;
}

void Mob::updateLocRot(float subDelta) {
	float angleDelta = currRotation - prevRotation;
	if (angleDelta > M_PI) {
		angleDelta -= 2.f * M_PI;
	} else if (angleDelta < -M_PI) {
		angleDelta += 2.f * M_PI;
	}

	sprite->setPosition(prevPosition + (currPosition - prevPosition) * subDelta);
	sprite->setRotation(prevRotation + angleDelta * subDelta);
}

GameplayState::GameplayState(Resources& r) : _resources(r) {
	std::cout << "entered gameplay state\n";
	ncine::theApplication().gfxDevice().setClearColor(ncine::Colorf(1, 1, 1, 1));
	auto& rootNode = ncine::theApplication().rootNode();
	this->cameraNode = std::make_unique<ncine::SceneNode>(&rootNode);
	this->LoadLevel();
	timeLeftNode = new ncine::TextNode(&rootNode, _resources.fonts["comic"].get());

	lastMessageReceivedTime = ncine::TimeStamp::now();
}

tweeny::tween<int>
CreateHidingSpotTween(ncine::DrawableNode* hspot, int from, int to, int during) {
	auto tween = tweeny::from(from)
		.to(to).during(during).onStep(
		[hspot] (tweeny::tween<int>& t, int v) -> bool {
			hspot->setAlpha(v);
			return false;
		}
	);
	return tween;
}

GameplayState::~GameplayState() {
}

StateType GameplayState::Update(Messages m) {
	for (auto& msg: m.data_msgs) {
		OnMessage(msg);
	}

	auto now = ncine::TimeStamp::now();
	float subDelta = (now.seconds() - lastMessageReceivedTime.seconds()) * ANIMATION_FPS;
	if (subDelta < 0.f) {
		subDelta = 0.f;
	} else if (subDelta > 1.f) {
		subDelta = 1.f;
	}

	// show highscores if necessary
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	if (this->showHighscores || gameEnded) {
		ImGui::Begin("Highscores", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse);
		ImGui::Columns(2);
		for (auto& p : gameData.players) {
			ImGui::Text("%s", p.name.c_str());
			ImGui::NextColumn();
			ImGui::Text("%s", std::to_string(p.score).c_str());
			ImGui::NextColumn();
		}
		ImGui::SetWindowPos({screenWidth/2 - 200.f, screenHeight/2 - 200.f});
		ImGui::SetWindowSize({400.f, 400.f});
		ImGui::End();
	}

	// Update mob positions
	for (auto& mob : this->mobs) {
		mob.second.updateLocRot(subDelta);
	}

	if (this->mySprite == nullptr)
		return StateType::Gameplay;

	// manage the screen position in relation to my sprite
	auto &mouseState = ncine::theApplication().inputManager().mouseState();
	auto myMobPosition = -this->mySprite->position() +
		ncine::Vector2f(3*screenWidth/4 - mouseState.x/2,
						screenHeight/4 + mouseState.y/2);
	this->cameraNode->setPosition(myMobPosition);

	// hovering
	ncine::Vector2f mouseCoords;
	mouseCoords.x = this->mySprite->position().x + (mouseState.x - screenWidth / 2) * 1.5f;
	mouseCoords.y = this->mySprite->position().y + -(mouseState.y - screenHeight / 2) * 1.5f;
	float radiusSquared = this->mySprite->size().x/2;
	radiusSquared = radiusSquared*radiusSquared;

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
		closestMob->hoverMarker = std::make_unique<ncine::Sprite>(closestMob->sprite.get(), _resources.textures["graycircle.png"].get());
		closestMob->hoverMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
	}

	// hiding spot transparency
	for (auto &&hspot : this->hiding_spots)
	{
		if (!this->mySprite) break;
		auto distance = this->mySprite->position() - hspot->position();
		auto radius = ncine::Vector2f(hspot->width()/2.0f, hspot->height()/2.0f);
		auto radius_offset = ncine::Vector2f(this->mySprite->height()/4.f, this->mySprite->height()/4.f); // uwzględnienie odległości krawędzi od jego środka
		radius += radius_offset;
		// FIXME: [future] równanie elipsy jest w poziomie w tej chwili, jak krzak będzie podłużny i obrócony, to nie będzie działało
		if ((distance.x*distance.x)/(radius.x*radius.x) + (distance.y*distance.y)/(radius.y*radius.y) <= 1) { //równanie elipsy, dziwki
			if (hspot->alpha() == MAX_HIDING_SPOT_OPACITY) {
			auto tween = CreateHidingSpotTween(hspot.get(), MAX_HIDING_SPOT_OPACITY, MIN_HIDING_SPOT_OPACITY, 10);
			_resources._tweens.push_back(tween);
			}
		} else if (hspot->alpha() == MIN_HIDING_SPOT_OPACITY) {
			auto tween = CreateHidingSpotTween(hspot.get(), MIN_HIDING_SPOT_OPACITY, MAX_HIDING_SPOT_OPACITY, 20);
			_resources._tweens.push_back(tween);
		}
	}

	return StateType::Gameplay;
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
	
	if (event.sym == ncine::KeySym::TAB)
		this->showHighscores = true;
}

void GameplayState::OnKeyReleased(const ncine::KeyboardEvent &event) {
	if (event.sym == ncine::KeySym::Q)
		SendCommandRun(false);

	if (event.sym == ncine::KeySym::TAB)
		this->showHighscores = false;
}

void GameplayState::updateRemainingText(uint64_t remainingFrames) {
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();

	if (remainingFrames > 0) {
		const uint64_t seconds = remainingFrames / uint64_t(ANIMATION_FPS);
		char timeStr[32];
		sprintf(timeStr, "%lu:%02lu", seconds / 60, seconds % 60);
		timeLeftNode->setString(timeStr);
		timeLeftNode->setColor(0, 0, 0, 255);
		timeLeftNode->setPosition(screenWidth, screenHeight);
		timeLeftNode->setAnchorPoint(1.f, 1.f);
		timeLeftNode->setAlpha(127);
		timeLeftNode->setScale(2.0f);
	} else {
		timeLeftNode->setString("Game over");
		timeLeftNode->setColor(255, 0, 0, 255);
		timeLeftNode->setPosition(screenWidth * 0.5f, screenHeight * 0.875f);
		timeLeftNode->setAnchorPoint(0.5f, 0.5f);
		timeLeftNode->setAlpha(255);
		timeLeftNode->setScale(4.0f);
	}
}
