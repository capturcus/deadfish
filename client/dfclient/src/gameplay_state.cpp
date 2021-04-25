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
	return ret;
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
tweeny::tween<int>
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
				shadowMesh.addChain(c);
			} else {
				fov::chain c;
				c.reserve(mask->polyverts()->size());
				for (auto v : *mask->polyverts()) {
					auto vv = ncine::Vector4f(v->x(), v->y(), 0.f, 1.f) * transform;
					c.push_back(ncine::Vector2f(vv.x, vv.y));
				}
				shadowMesh.addChain(c);
			}
		}
	}
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

template<typename T>
void resetMovableMap(std::map<uint16_t, T>& map)
{
	for (auto& it : map)
		it.second.seen = false;
}

template<typename T>
void updateMovableMap(std::map<uint16_t, T>& map, float subDelta)
{
	for (auto& it : map) {
		auto &mov = it.second;
		mov.tween.step(1);
		mov.lerp.updateLerp(subDelta);
	}
}

void GameplayState::ProcessWorldState(const void* ev) {
	auto worldState = (const FlatBuffGenerated::WorldState*) ev;
	resetMovableMap(this->mobs);
	resetMovableMap(this->inkParticles);
	resetMovableMap(this->manipulators);

	for (int i = 0; i < worldState->mobs()->size(); i++) {
		auto mobData = worldState->mobs()->Get(i);
		processMovable(this->mobs, mobData->movable(), [&](){
			Mob ret;
			ret.sprite = CreateNewMobSprite(this->cameraNode.get(), mobData->species());
			return ret;
		});
		Mob& mob = this->mobs.find(mobData->movable()->ID())->second; // after processMovable this is guaranteed to exist in the map
		if (mobData->state() != mob.state) {
			ncine::AnimatedSprite* animSprite = dynamic_cast<ncine::AnimatedSprite*>(mob.sprite.get());
			mob.state = mobData->state();
			animSprite->setAnimationIndex(mobData->state());
			animSprite->setFrame(0);
			animSprite->setPaused(false);
		}
		// if it's us then save sprite
		if (mob.movableID == gameData.myMobID)
			this->mySprite = mob.sprite.get();

		if (mobData->relation() == FlatBuffGenerated::PlayerRelation_None) {
			mob.relationMarker.reset(nullptr);
		} else if (mobData->relation() == FlatBuffGenerated::PlayerRelation_Targeted) {
			mob.relationMarker = std::make_unique<ncine::Sprite>(mob.sprite.get(), _resources.textures["redcircle.png"].get());
			mob.relationMarker->setColor(ncine::Colorf(1, 1, 1, 0.3));
			mob.relationMarker->setLayer((unsigned short)Layers::INDICATOR);
		}
	}

	deleteUnusedMovables(this->mobs);

	if (this->mobs.find(gameData.myMobID) == this->mobs.end()) {
		this->mySprite = nullptr;
		return;
	}

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
				_resources._intTweens.push_back(tween);
			}
		}
	}
	if (worldState->currentHidingSpot()->str() != this->currentHidingSpot) {
		auto &hspotSprites = this->hiding_spots[this->currentHidingSpot];
		for (auto &hsSprite : hspotSprites) {
			auto tween = CreateAlphaTransitionTween(hsSprite.get(), MIN_HIDING_SPOT_OPACITY, MAX_HIDING_SPOT_OPACITY, 20);
			_resources._intTweens.push_back(tween);
		}
	}
	this->currentHidingSpot = worldState->currentHidingSpot()->str();

	for (int i = 0; i < worldState->inkParticles()->size(); i++) {
		auto inkParticleData = worldState->inkParticles()->Get(i);
		processMovable(this->inkParticles, inkParticleData->movable(), [&](){
			InkParticle newInk;
			int inkNum = rand() % 3 + 1;
			std::string inkTexName = std::string("ink") + std::to_string(inkNum) + ".png";
			newInk.sprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[inkTexName].get());
			newInk.sprite->setLayer((unsigned short) Layers::INK_PARTICLES);
			newInk.sprite->setScale(120./330.); // todo: fix my life
			return newInk;
		});
	}

	deleteUnusedMovables(this->inkParticles);

	for (int i = 0; i < worldState->mobManipulators()->size(); i++) {
		auto manipulatorData = worldState->mobManipulators()->Get(i);
		processMovable(this->manipulators, manipulatorData->movable(), [&](){
			auto manipTexture = manipulatorData->type() == FlatBuffGenerated::MobManipulatorType_Dispersor ?
				"dispersor.png" : "attractor.png";
			auto sprite = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures[manipTexture].get());
			sprite->setLayer((unsigned short) Layers::MOB_MANIPULATORS);
			Manipulator ret;
			ret.sprite = std::move(sprite);
			return ret;
		});
	}

	deleteUnusedMovables(this->manipulators);
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

GameplayState::GameplayState(Resources& r) : GameState(r), _deathReportProcessor(r) {
	std::cout << "entered gameplay state\n";
	ncine::theApplication().gfxDevice().setClearColor(ncine::Colorf(1, 1, 1, 1));
	auto& rootNode = ncine::theApplication().rootNode();
	this->cameraNode = std::make_unique<ncine::SceneNode>(&rootNode);
	this->LoadLevel();
	timeLeftNode = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());

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
	// the order of these is important
	// because the ones that are higher contain references to the ones that are lower
	// and nCine tries to free all the memory that is has a pointer to for some reason
	// which leads to double frees if the order is not preserved
	nodes.clear();
	textNodes.clear();
	hiding_spots.clear();
	destination_marker.reset();
	skillIcons.clear();
	shadowNode.reset();
	mobs.clear();
	cameraNode.reset();
}

void GameplayState::updateHovers(ncine::Vector2f mouseCoords, float radiusSquared) {
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
}

StateType GameplayState::Update(Messages m) {
	if (m.closed) {
		gameData.gameClosed = true;
		return StateType::Menu;
	}
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

	updateMovableMap(this->mobs, subDelta);
	updateMovableMap(this->inkParticles, subDelta);
	updateMovableMap(this->manipulators, subDelta);

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

	updateHovers(mouseCoords, radiusSquared);

	// clean up transparent text
	auto new_end = std::remove_if(textNodes.begin(), textNodes.end(), [] (auto& t){
		return t->alpha() == 0;
	});
	this->textNodes.resize(std::distance(textNodes.begin(), new_end));

	return StateType::Gameplay;
}

void GameplayState::ProcessDeathReport(const void* ev) {
	auto deathReport = (const FlatBuffGenerated::DeathReport*) ev;
	auto newTextNodes = _deathReportProcessor.ProcessDeathReport(deathReport);
	std::move(newTextNodes.begin(), newTextNodes.end(), std::back_inserter(this->textNodes));
}

void GameplayState::OnMouseButtonPressed(const ncine::MouseEvent &event) {
	if (this->mySprite == nullptr)
		return;
	if (event.isLeftButton()) {
		// move
		const float screenWidth = ncine::theApplication().width();
		const float screenHeight = ncine::theApplication().height();
		const float worldX = this->mySprite->position().x + (event.x - screenWidth / 2) * 1.5f;
		const float worldY = this->mySprite->position().y + (event.y - screenHeight / 2) * 1.5f;
		const float serverX = worldX * PIXELS2METERS;
		const float serverY = -worldY * PIXELS2METERS;
		flatbuffers::FlatBufferBuilder builder;
		auto pos = FlatBuffGenerated::Vec2(serverX, serverY);
		auto cmdMove = FlatBuffGenerated::CreateCommandMove(builder, &pos);
		auto message = FlatBuffGenerated::CreateClientMessage(builder, FlatBuffGenerated::ClientMessageUnion_CommandMove, cmdMove.Union());
		builder.Finish(message);
		SendData(builder);

		auto player = dynamic_cast<ncine::AnimatedSprite*>(this->mySprite);
		// if player is killing sb at the moment, don't create destination marker
		if (player->animationIndex() == FlatBuffGenerated::MobState_Attack) return;
		this->destination_marker = std::make_unique<ncine::Sprite>(this->cameraNode.get(), _resources.textures["destmarker.png"].get());
		this->destination_marker->setPosition(worldX, worldY);
		this->destination_marker->setLayer((unsigned short)Layers::INDICATOR);
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
				this->destination_marker = nullptr;
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

	const auto outline = shadowMesh.calculateOutline(ncine::Vector2f(origin.x, -origin.y) * PIXELS2METERS);
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
