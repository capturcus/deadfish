#include <complex>
#include <functional>
#include <iostream>
#include <fstream>

#include <ncine/FileSystem.h>
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

const int TILE_WIDTH = 128;
const int TILE_HEIGHT = 128;

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

void GameplayState::LoadLevel() {
	auto level = FBUtilGetServerEvent(gameData.levelData, Level);

	// build (guid->sprite) map
	std::map<uint16_t, std::string> spritemap;

	for (auto fb_Ti : *level->tileinfo()) {
		spritemap.insert(std::pair<uint16_t, std::string>(fb_Ti->gid(), fb_Ti->name()->str()));
	}

	// initialize tile layer
	if (level->tilelayer()) {
		auto width = level->tilelayer()->width();
		auto height = level->tilelayer()->height();
		std::vector<std::string> rows;
		
		std::stringstream ss(level->tilelayer()->data()->str());
		std::string row;
		while (std::getline(ss, row, '\n')) {
			if(row.empty()) continue; // first row is empty
			rows.push_back(row);
		}

		for (int i=0; i<height; ++i) {
			std::vector<std::string> ids;

			std::stringstream ss2(rows[i]);
			std::string id;
			while (std::getline(ss2, id, ',')) {
				ids.push_back(id);
			}

			for (int j=0; j<width; ++j) {
				auto spritename = spritemap[std::stoi(ids[j])];
				if (spritename.empty()) continue;
				auto tileSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[spritename].get(),
					j * TILE_WIDTH, -i * TILE_HEIGHT);
				tileSprite->setAnchorPoint(0, 1);
				tileSprite->setLayer(TILE_LAYER);
				this->nodes.push_back(std::move(tileSprite));
			}
		}
	}

	// initialize decoration
	for (auto decoration : *level->decoration()) {
		std::string spritename = spritemap[decoration->gid()];
		auto decorationSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[spritename].get(),
			decoration->pos()->x() * METERS2PIXELS, -decoration->pos()->y() * METERS2PIXELS);
		decorationSprite->setAnchorPoint(0, 1);
		decorationSprite->setRotation(-decoration->rotation());
		decorationSprite->setLayer(DECORATION_LAYER);
		this->nodes.push_back(std::move(decorationSprite));
	}
	
	// initialize objects
	for (auto object : *level->objects()) {
		std::string spritename = spritemap[object->gid()];
		auto objectSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[spritename].get(),
			object->pos()->x() * METERS2PIXELS, -object->pos()->y() * METERS2PIXELS);
		objectSprite->setAnchorPoint(0, 1);
		objectSprite->setRotation(-object->rotation());
		if(object->hspotname()->str().empty()){
			objectSprite->setLayer(OBJECTS_LAYER);
			this->nodes.push_back(std::move(objectSprite));
		} else {
			objectSprite->setLayer(HIDING_SPOTS_LAYER);
			auto hspotgroup = this->hiding_spots.find(object->hspotname()->str());
			if (hspotgroup != this->hiding_spots.end()) { // if found group, add to group
				hspotgroup->second.push_back(std::move(objectSprite));
			} else {	// if no group, create group
				DrawableNodeVector vector;
				vector.push_back(std::move(objectSprite));
				std::pair<std::string, DrawableNodeVector> pair(object->hspotname()->str(), std::move(vector));
				this->hiding_spots.insert(std::move(pair));
			}
		}

	}
}

// this whole thing should probably be refactored
void GameplayState::ProcessDeathReport(const FlatBuffGenerated::DeathReport* deathReport) {
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

void GameplayState::ProcessHighscoreUpdate(const FlatBuffGenerated::HighscoreUpdate* highscoreUpdate) {
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
		if (simpleServerEvent->type() == FlatBuffGenerated::SimpleServerEventType_GameEnded) {
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
		bool firstUpdate = false;
		if (mobItr == this->mobs.end()) {
			// this is the first time we see this mob, create it
			Mob newMob;
			newMob.sprite = CreateNewAnimSprite(this->cameraNode.get(), mobData->species());
			this->mobs[mobData->mobID()] = std::move(newMob);
			mobItr = this->mobs.find(mobData->mobID());
			firstUpdate = true;
		}
		Mob& mob = mobItr->second;
		mob.setupLocRot(*mobData, firstUpdate);
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

		if (mobData->relation() == FlatBuffGenerated::PlayerRelation_None) {
			mob.relationMarker.reset(nullptr);
		} else if (mobData->relation() == FlatBuffGenerated::PlayerRelation_Close) {
			mob.relationMarker = std::make_unique<ncine::Sprite>(mob.sprite.get(), _resources.textures["bluecircle.png"].get());
			mob.relationMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
		} else if (mobData->relation() == FlatBuffGenerated::PlayerRelation_Targeted) {
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

	// make current hidingspot transparent
	if(worldState->currentHidingSpot()->str() != "") {
		auto &hspotSprites = this->hiding_spots[worldState->currentHidingSpot()->str()];
		if (!hspotSprites.empty() && hspotSprites[0]->alpha() == MAX_HIDING_SPOT_OPACITY) {
			for (auto &hsSprite : hspotSprites) {
				auto tween = CreateHidingSpotTween(hsSprite.get(), MAX_HIDING_SPOT_OPACITY, MIN_HIDING_SPOT_OPACITY, 10);
				_resources._tweens.push_back(tween);
			}
		}
	}
	if (worldState->currentHidingSpot()->str() != this->currentHidingSpot) {
		auto &hspotSprites = this->hiding_spots[this->currentHidingSpot];
		for (auto &hsSprite : hspotSprites) {
			auto tween = CreateHidingSpotTween(hsSprite.get(), MIN_HIDING_SPOT_OPACITY, MAX_HIDING_SPOT_OPACITY, 20);
			_resources._tweens.push_back(tween);
		}
	}
	this->currentHidingSpot = worldState->currentHidingSpot()->str();
}

void Mob::setupLocRot(const FlatBuffGenerated::Mob& msg, bool firstUpdate) {
	prevPosition = currPosition;
	prevRotation = currRotation;

	currPosition.x = msg.pos()->x() * METERS2PIXELS;
	currPosition.y = -msg.pos()->y() * METERS2PIXELS;
	currRotation = -msg.angle() * 180.f / M_PI;

	if (firstUpdate) {
		prevPosition = currPosition;
		prevRotation = currRotation;
	}
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

	// show quit dialog if necessary
	if (this->showQuitDialog) {
		ImGui::Begin("Do you really want to quit?", &this->showQuitDialog, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse);
		ImGui::SetCursorPosX(70);
		ImGui::SetCursorPosY(30);

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(7.0f, 0.0f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(7.0f, 0.0f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(7.0f, 0.0f, 0.8f));
		if(ImGui::Button("No")) {
			this->showQuitDialog = false;
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::SetCursorPosX(115);

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(7.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(7.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(7.0f, 0.8f, 0.8f));
		if (ImGui::Button("Yes")) {
			nc::theApplication().quit();
		}
		ImGui::PopStyleColor(3);

		ImGui::SetWindowPos({screenWidth/2 - 110.f, screenHeight/2 - 30.f});
		ImGui::SetWindowSize({220, 60.f});
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
		auto pos = FlatBuffGenerated::Vec2(serverX, serverY);
		auto cmdMove = FlatBuffGenerated::CreateCommandMove(builder, &pos);
		auto message = FlatBuffGenerated::CreateClientMessage(builder, FlatBuffGenerated::ClientMessageUnion_CommandMove, cmdMove.Union());
		builder.Finish(message);
		SendData(builder);
	}
	if (event.isRightButton()) {
		// kill
		for (auto& mob : this->mobs) {
			if (mob.second.hoverMarker.get()) {
				// if it is hovered kill it
				flatbuffers::FlatBufferBuilder builder;
				auto cmdKill = FlatBuffGenerated::CreateCommandKill(builder, mob.first);
				auto message = FlatBuffGenerated::CreateClientMessage(builder, FlatBuffGenerated::ClientMessageUnion_CommandKill, cmdKill.Union());
				builder.Finish(message);
				SendData(builder);
				break;
			}
		}
	}
}

void SendCommandRun(bool run) {
	flatbuffers::FlatBufferBuilder builder;
	auto cmdRun = FlatBuffGenerated::CreateCommandRun(builder, run);
	auto message = FlatBuffGenerated::CreateClientMessage(builder, FlatBuffGenerated::ClientMessageUnion_CommandRun, cmdRun.Union());
	builder.Finish(message);
	SendData(builder);
}

void GameplayState::OnKeyPressed(const ncine::KeyboardEvent &event) {
	if (event.sym == ncine::KeySym::Q)
		SendCommandRun(true);
	
	if (event.sym == ncine::KeySym::TAB)
		this->showHighscores = true;

	if (event.sym == ncine::KeySym::ESCAPE)
		this->showQuitDialog = !this->showQuitDialog;
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
