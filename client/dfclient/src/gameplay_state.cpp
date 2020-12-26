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

const int MAX_HIDING_SPOT_OPACITY = 255;
const int MIN_HIDING_SPOT_OPACITY = 128;

const int MOB_FADEIN_TIME = 5;
const int MOB_FADEOUT_TIME = 7;

std::map<uint16_t, std::string> skillTextures = {
	{(uint16_t) Skills::INK_BOMB, "skill_ink.png"},
	{(uint16_t) Skills::ATTRACTOR, "skill_attractor.png"},
	{(uint16_t) Skills::DISPERSOR, "skill_dispersor.png"},
	{(uint16_t) Skills::BLINK, "skill_blink.png"},
};

using handler_t = void (GameplayState::*)(const void*);
static handler_t messageHandlers[FlatBuffGenerated::ServerMessageUnion_MAX + 1];

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

std::unique_ptr<ncine::AnimatedSprite> GameplayState::CreateNewAnimSprite(ncine::SceneNode* parent, uint16_t species, const std::string& spritesheet, uint16_t maxAnimations, Layers layer) {
	std::unique_ptr<ncine::AnimatedSprite> ret = std::make_unique<ncine::AnimatedSprite>(parent, _resources.textures[spritesheet].get());
	int currentImg = species * IMGS_PER_SPECIES;
	for (int animNumber = 0; animNumber < maxAnimations; animNumber++) {
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

	ret->setAnimationIndex(0);
	ret->setFrame(0);
	ret->setPaused(false);
	ret->setLayer((unsigned short)layer);
	return std::move(ret);
}

std::unique_ptr<ncine::AnimatedSprite> GameplayState::CreateNewMobSprite(ncine::SceneNode* parent, uint16_t species) {
	if (species == GOLDFISH_SPECIES) {
		std::string goldfish("goldfish.png");
		return this->CreateNewAnimSprite(parent, 0, goldfish, 1, Layers::MOBS);
	}
	std::string fish("fish.png");
	return this->CreateNewAnimSprite(parent, species, fish, FISH_ANIMATIONS::MAX, Layers::MOBS);
}

/**
 * Creates a tween making a sprite fade through from one alpha value to another (specified on a 0-255 scale)
 * @param sprite	the sprite to change
 * @param from 		the initial alpha value
 * @param to 		the final alpha value
 * @param during	time it takes
 * @return 			the created tween
*/
static tweeny::tween<int>
CreateAlphaTransitionTween(ncine::DrawableNode* sprite, int from, int to, int during) {
	auto tween = tweeny::from(from)
		.to(to).during(during).onStep(
		[sprite] (tweeny::tween<int>& t, int v) -> bool {
			sprite->setAlpha(v);
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
	if (level->tilelayer() && level->tilelayer()->tiledata()) {
		auto width = level->tilelayer()->width();
		auto height = level->tilelayer()->height();
		auto tilewidth = level->tilelayer()->tilesize()->x();
		auto tileheight = level->tilelayer()->tilesize()->y();
		
		auto currentTile = level->tilelayer()->tiledata()->begin();
		for (int i=0; i<height; ++i) {	// rows
			for (int j=0; j<width; ++j) {	// columns
				auto spritename = spritemap[*currentTile++];
				if (spritename.empty()) continue;
				auto tileSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[spritename].get(),
					j * tilewidth, -i * tileheight);
				tileSprite->setAnchorPoint(0, 1);
				tileSprite->setLayer((unsigned short)Layers::TILE);
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
		decorationSprite->setSize(decoration->size()->x(), decoration->size()->y());
		decorationSprite->setLayer((unsigned short)Layers::DECORATION);
		this->nodes.push_back(std::move(decorationSprite));
	}
	
	// initialize objects
	for (auto object : *level->objects()) {
		std::string spritename = spritemap[object->gid()];
		auto objectSprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[spritename].get(),
			object->pos()->x() * METERS2PIXELS, -object->pos()->y() * METERS2PIXELS);
		objectSprite->setAnchorPoint(0, 1);
		objectSprite->setRotation(-object->rotation());
		objectSprite->setSize(object->size()->x(), object->size()->y());
		if (object->hspotname()->str().empty()){
			objectSprite->setLayer((unsigned short)Layers::OBJECTS);
			this->nodes.push_back(std::move(objectSprite));
		} else {
			objectSprite->setLayer((unsigned short)Layers::HIDING_SPOTS);
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

	// Initialize shadow mesh
	if (level->collisionMasks() != nullptr) {
		for (auto mask : *level->collisionMasks()) {
			auto transform =
				ncine::Matrix4x4f::translation({mask->pos()->x(), mask->pos()->y(), 0.f}) *
				ncine::Matrix4x4f::rotationZ(mask->rotation());

			if (mask->isCircle()) {
				fov::circle c;
				c.pos.x = mask->pos()->x();
				c.pos.y = mask->pos()->y();
				c.radius = mask->size()->x() * 0.5f;
				shadowMesh.add_circle(c);
			} else {
				fov::chain c;
				c.reserve(mask->polyverts()->size());
				for (auto v : *mask->polyverts()) {
					auto vv = ncine::Vector4f(v->x(), v->y(), 0.f, 1.f) * transform;
					c.push_back(ncine::Vector2f(vv.x, vv.y));
				}
				shadowMesh.add_chain(c);
			}
		}
	}
}

// this whole thing should probably be refactored
void GameplayState::ProcessDeathReport(const void* ev) {
	auto deathReport = (const FlatBuffGenerated::DeathReport*) ev;
	auto& rootNode = ncine::theApplication().rootNode();
	auto text = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());
	auto outline = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic_outline"].get());
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	if (deathReport->killer() == gameData.myPlayerID) {
		// i killed someone
		std::string killedName;
		if (deathReport->killed() == (uint16_t)-1) {
			// it was an npc
			killedName = "a civilian";
			text->setColor(ncine::Color::Black);
		} else {
			// it was a player
			text->setColor(0, 255, 0, 255);
			killedName = gameData.players[deathReport->killed()].name + "!";
		}
		text->setString(("you killed " + killedName).c_str());
		text->setPosition(screenWidth * 0.5f, screenHeight * 0.75f);
		text->setScale(1.5f);
		_resources._tweens.push_back(CreateTextTween(text.get()));

		_resources.playKillSound();
		
	} else if (deathReport->killed() == gameData.myPlayerID) {
		// i died :c
		_resources.playKillSound(0.4f);
		_resources.playRandomDeathSound();

		text->setString(("you have been killed by " + gameData.players[deathReport->killer()].name).c_str());
		text->setColor(255, 0, 0, 255);
		text->setPosition(screenWidth * 0.5f, screenHeight * 0.5f);
		text->setScale(1.0f);
		_resources._tweens.push_back(CreateTextTween(text.get()));
	} else {
		auto killer = gameData.players[deathReport->killer()].name;
		auto killed = gameData.players[deathReport->killed()].name;
		text->setString((killer + " killed " + killed).c_str());
		text->setScale(0.25f);
		text->setPosition(screenWidth * 0.8f, screenHeight * 0.8f);
		text->setColor(0, 0, 0, 255);
		_resources._tweens.push_back(CreateTextTween(text.get()));
	}

	outline->setString(text->string());
	outline->setPosition(text->position());
	outline->setScale(text->scale());
	outline->setColor(0, 0, 0, 255);
	_resources._tweens.push_back(CreateTextTween(outline.get()));

	if (!(text->color() == ncine::Color::Black)) this->nodes.push_back(std::move(outline));
	this->nodes.push_back(std::move(text));
}

void GameplayState::ProcessHighscoreUpdate(const void* ev) {
	auto highscoreUpdate = (const FlatBuffGenerated::HighscoreUpdate*) ev;
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
	arc->setLayer((unsigned short)Layers::INDICATOR);
	arc->setRotation(-this->mySprite->rotation() - angle * TO_DEGREES - force * 180.f + 180.f);
	if (visible)
		arc->setColor(255, 255, 255, INDICATOR_OPACITY * 255);
	else
		arc->setColor(0, 0, 0, INDICATOR_OPACITY * 255);
	return arc;
}

void GameplayState::ProcessSimpleServerEvent(const void* ev) {
	auto event = (const FlatBuffGenerated::SimpleServerEvent*) ev;
	if (event->type() == FlatBuffGenerated::SimpleServerEventType_GameEnded) {
		gameEnded = true;
		updateRemainingText(0);
	}
}

void GameplayState::ProcessWorldState(const void* ev) {
	auto worldState = (const FlatBuffGenerated::WorldState*) ev;
	// reset seen status of mobs
	for (auto& p : this->mobs)
		p.second.seen = false;

	for (int i = 0; i < worldState->mobs()->size(); i++) {
		auto mobData = worldState->mobs()->Get(i);
		auto mobItr = this->mobs.find(mobData->mobID());
		if (mobItr == this->mobs.end()) {
			// this is the first time we see this mob, create it
			Mob newMob;
			newMob.sprite = CreateNewMobSprite(this->cameraNode.get(), mobData->species());
			newMob.lerp.bind(static_cast<ncine::DrawableNode*>(newMob.sprite.get()));

			//fade in
			newMob.sprite->setAlpha(1);
			_resources._mobTweens[mobData->mobID()] = CreateAlphaTransitionTween(newMob.sprite.get(), 1, 255, MOB_FADEIN_TIME);

			this->mobs[mobData->mobID()] = std::move(newMob);
			mobItr = this->mobs.find(mobData->mobID());
		}
		Mob& mob = mobItr->second;
		mob.lerp.setupLerp(mobData->pos()->x(), mobData->pos()->y(), mobData->angle());
		mob.seen = true;
		if (mob.isAfterimage) {
			//re-fade in
			_resources._mobTweens[mobData->mobID()] = CreateAlphaTransitionTween(mob.sprite.get(), mob.sprite->alpha(), 255, MOB_FADEIN_TIME*(1 - mob.sprite->alpha()/255));
			mob.isAfterimage = false;
		}
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
		} else if (mobData->relation() == FlatBuffGenerated::PlayerRelation_Targeted) {
			mob.relationMarker = std::make_unique<ncine::Sprite>(mob.sprite.get(), _resources.textures["redcircle.png"].get());
			mob.relationMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
			mob.relationMarker->setLayer((unsigned short)Layers::INDICATOR);
		}
	}
	std::vector<int> deletedIDs;
	for (auto& mob : this->mobs) {
		if (!mob.second.seen) { // not seen
			if (!mob.second.isAfterimage) {	// not seen and not afterimage
				// fade out
				_resources._mobTweens[mob.first] = CreateAlphaTransitionTween(mob.second.sprite.get(), mob.second.sprite->alpha(), 0, MOB_FADEOUT_TIME*(mob.second.sprite->alpha()/255));
				mob.second.isAfterimage = true;
			} else if (mob.second.sprite->alpha() == 0) { // not seen and afterimage and alpha == 0
				deletedIDs.push_back(mob.first);
			}
		}
	}
	for (auto id : deletedIDs) {
		this->mobs.erase(id);
		_resources._mobTweens.erase(id);
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
	if (worldState->currentHidingSpot()->str() != "") {
		auto &hspotSprites = this->hiding_spots[worldState->currentHidingSpot()->str()];
		if (!hspotSprites.empty() && hspotSprites[0]->alpha() == MAX_HIDING_SPOT_OPACITY) {
			for (auto &hsSprite : hspotSprites) {
				auto tween = CreateAlphaTransitionTween(hsSprite.get(), MAX_HIDING_SPOT_OPACITY, MIN_HIDING_SPOT_OPACITY, 10);
				_resources._tweens.push_back(tween);
			}
		}
	}
	if (worldState->currentHidingSpot()->str() != this->currentHidingSpot) {
		auto &hspotSprites = this->hiding_spots[this->currentHidingSpot];
		for (auto &hsSprite : hspotSprites) {
			auto tween = CreateAlphaTransitionTween(hsSprite.get(), MIN_HIDING_SPOT_OPACITY, MAX_HIDING_SPOT_OPACITY, 20);
			_resources._tweens.push_back(tween);
		}
	}
	this->currentHidingSpot = worldState->currentHidingSpot()->str();

	// handle ink particles
	for (auto& i : this->inkParticles)
		i.second.seen = false;

	for (int i = 0; i < worldState->inkParticles()->size(); i++) {
		auto ink = worldState->inkParticles()->Get(i);
		auto inkItr = inkParticles.find(ink->inkID());
		if (inkItr == inkParticles.end()) {
			// not found, make a new one
			InkParticle newInk;
			int inkNum = rand() % 3 + 1;
			std::string inkTexName = std::string("ink") + std::to_string(inkNum) + ".png";
			newInk.sprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[inkTexName].get());
			newInk.sprite->setLayer((unsigned short) Layers::INK_PARTICLES);
			newInk.sprite->setScale(120./330.); // todo: fix my life

			inkParticles.insert({ink->inkID(), std::move(newInk)});
			inkItr = inkParticles.find(ink->inkID());
			inkItr->second.lerp.bind(inkItr->second.sprite.get());
		}
		inkItr->second.lerp.setupLerp(ink->pos()->x(), ink->pos()->y(), inkItr->second.sprite->rotation());
		inkItr->second.seen = true;
	}

	for (auto it = this->inkParticles.begin(); it != this->inkParticles.end();) {
		if (!it->second.seen)
			it = this->inkParticles.erase(it);
		else
			++it;
	}

	// handle mob manipulators
	this->manipulators.clear();
	for (int i = 0; i < worldState->mobManipulators()->size(); i++) {
		auto manipulator = worldState->mobManipulators()->Get(i);
		auto manipTexture = manipulator->type() == FlatBuffGenerated::MobManipulatorType_Dispersor ?
			"dispersor.png" : "attractor.png";
		auto sprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[manipTexture].get());
		sprite->setLayer((unsigned short) Layers::MOB_MANIPULATORS);
		sprite->setPosition({manipulator->pos()->x() * METERS2PIXELS, -manipulator->pos()->y() * METERS2PIXELS});
		this->manipulators.push_back(std::move(sprite));
	}
}

void GameplayState::ProcessSkillBarUpdate(const void* ev) {
	std::cout << "skill bar update received\n";
	auto skillBarUpdate = (const FlatBuffGenerated::SkillBarUpdate*) ev;
	this->skillIcons.clear();
	auto& rootNode = ncine::theApplication().rootNode();
	for (int i = 0; i < skillBarUpdate->skills()->size(); i++) {
		uint16_t skill = skillBarUpdate->skills()->Get(i);
		auto skillTexture = skillTextures[skill];
		auto skillSprite = std::make_unique<ncine::Sprite>(&rootNode, _resources.textures[skillTexture].get(),
			i * 150 + 100, 100);
		skillSprite->setScale(0.4f);
		skillSprite->setLayer((unsigned short) Layers::SKILLS);
		this->skillIcons.push_back(std::move(skillSprite));
	}
}

void GameplayState::OnMessage(const std::string& data) {
	lastMessageReceivedTime = ncine::TimeStamp::now();

	auto serverMessage = flatbuffers::GetRoot<FlatBuffGenerated::ServerMessage>(data.data());
	auto type = serverMessage->event_type();
	if (type < 0 || type > FlatBuffGenerated::ServerMessageUnion_MAX) {
		std::cout << "wrong message type " << type << "\n";
		return;
	}

	auto handler = messageHandlers[type];
	if (!handler) {
		std::cout << "could not find a handler for type " << type << "\n";
		return;
	}
	// this is the ugliest cpp line i've ever written
	(this->*handler)(serverMessage->event());
}


GameplayState::GameplayState(Resources& r) : _resources(r) {
	std::cout << "entered gameplay state\n";
	ncine::theApplication().gfxDevice().setClearColor(ncine::Colorf(1, 1, 1, 1));
	auto& rootNode = ncine::theApplication().rootNode();
	this->cameraNode = std::make_unique<ncine::SceneNode>(&rootNode);
	this->LoadLevel();
	timeLeftNode = new ncine::TextNode(&rootNode, _resources.fonts["comic"].get());

	lastMessageReceivedTime = ncine::TimeStamp::now();

	messageHandlers[FlatBuffGenerated::ServerMessageUnion_HighscoreUpdate] =
		&GameplayState::ProcessHighscoreUpdate;
	messageHandlers[FlatBuffGenerated::ServerMessageUnion_DeathReport] =
		&GameplayState::ProcessDeathReport;
	messageHandlers[FlatBuffGenerated::ServerMessageUnion_SimpleServerEvent] =
		&GameplayState::ProcessSimpleServerEvent;
	messageHandlers[FlatBuffGenerated::ServerMessageUnion_WorldState] =
		&GameplayState::ProcessWorldState;
	messageHandlers[FlatBuffGenerated::ServerMessageUnion_SkillBarUpdate] =
		&GameplayState::ProcessSkillBarUpdate;
}

GameplayState::~GameplayState() {
}

StateType GameplayState::Update(Messages m) {
	for (auto& msg: m.data_msgs) {
		OnMessage(msg);
	}

	updateShadows();

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
		if (ImGui::Button("No")) {
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
		mob.second.lerp.updateLerp(subDelta);
	}

	// Update ink cloud positions
	for (auto& ip : this->inkParticles) {
		ip.second.lerp.updateLerp(subDelta);
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
		closestMob->hoverMarker->setLayer((unsigned short)Layers::INDICATOR);
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
			if (mob.second.isAfterimage) continue;
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

void GameplayState::TryUseSkill(uint8_t skillPos) {
	if (skillPos >= skillIcons.size())
		return;
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	auto &mouseState = ncine::theApplication().inputManager().mouseState();
	const float serverX = (this->mySprite->position().x + (mouseState.x - screenWidth / 2) * 1.5f) * PIXELS2METERS;
	const float serverY = -(this->mySprite->position().y - (mouseState.y - screenHeight / 2) * 1.5f) * PIXELS2METERS;
	flatbuffers::FlatBufferBuilder builder;
	FlatBuffGenerated::Vec2 mousePos{serverX, serverY};
	auto cmdSkill = FlatBuffGenerated::CreateCommandSkill(builder, skillPos, &mousePos);
	auto message = FlatBuffGenerated::CreateClientMessage(builder, FlatBuffGenerated::ClientMessageUnion_CommandSkill, cmdSkill.Union());
	builder.Finish(message);
	SendData(builder);
}

void GameplayState::OnKeyPressed(const ncine::KeyboardEvent &event) {
	if (event.sym == ncine::KeySym::Q)
		SendCommandRun(true);
	
	if (event.sym == ncine::KeySym::TAB)
		this->showHighscores = true;
	
	if (event.sym == ncine::KeySym::N1)
		this->TryUseSkill(0);
	if (event.sym == ncine::KeySym::N2)
		this->TryUseSkill(1);
	if (event.sym == ncine::KeySym::N3)
		this->TryUseSkill(2);

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
		timeLeftNode->setScale(1.0f);
	} else {
		timeLeftNode->setString("Game over");
		timeLeftNode->setColor(255, 0, 0, 255);
		timeLeftNode->setPosition(screenWidth * 0.5f, screenHeight * 0.875f);
		timeLeftNode->setAnchorPoint(0.5f, 0.5f);
		timeLeftNode->setAlpha(255);
		timeLeftNode->setScale(2.0f);
	}
}

void GameplayState::updateShadows() {
	if (!mySprite) {
		shadowNode.reset();
		return;
	}

	const auto camPos = this->cameraNode->absPosition();
	auto origin = mySprite->position();

	const auto outline = shadowMesh.calculate_outline(ncine::Vector2f(origin.x, -origin.y) * PIXELS2METERS);
	std::vector<ncine::MeshSprite::Vertex> strip;
	strip.resize(outline.size() * 6);

	origin += camPos;

	for (const auto& seg : outline) {
		ncine::MeshSprite::Vertex a, b, c, d;
		auto first = ncine::Vector2f(seg.first.x, -seg.first.y) * METERS2PIXELS + camPos;
		auto second = ncine::Vector2f(seg.second.x, -seg.second.y) * METERS2PIXELS + camPos;
		a.x = first.x;
		a.y = first.y;
		b.x = second.x;
		b.y = second.y;
		auto diffFirst = first - origin;
		if (diffFirst.sqrLength() > 0.f) {
			diffFirst *= 10000.f / diffFirst.length();
		}
		auto diffSecond = second - origin;
		if (diffSecond.sqrLength() > 0.f) {
			diffSecond *= 10000.f / diffSecond.length();
		}
		c.x = origin.x + diffFirst.x;
		c.y = origin.y + diffFirst.y;
		d.x = origin.x + diffSecond.x;
		d.y = origin.y + diffSecond.y;

		// Triangles
		strip.push_back(a);
		strip.push_back(a);
		strip.push_back(b);
		strip.push_back(c);
		strip.push_back(d);
		strip.push_back(d);
	}

	if (shadowNode == nullptr) {
		auto& rootNode = ncine::theApplication().rootNode();
		shadowNode = std::unique_ptr<ncine::MeshSprite>(new ncine::MeshSprite(&rootNode, _resources.textures["blackpixel.png"].get()));
		shadowNode->setLayer((unsigned short)Layers::SHADOW);
		shadowNode->setAlphaF(0.75f);
	}
	shadowNode->setVertices(strip.size(), strip.data());
}
