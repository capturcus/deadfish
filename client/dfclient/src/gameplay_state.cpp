#include <iostream>
#include <functional>

#include <ncine/Application.h>
#include <ncine/Colorf.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>
#include <ncine/AnimatedSprite.h>

#include "gameplay_state.hpp"
#include "game_data.hpp"
#include "fb_util.hpp"
#include "state_manager.hpp"

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

void GameplayState::OnMessage(const std::string& data) {
    auto worldState = FBUtilGetServerEvent(data, WorldState);
    if (!worldState)
        return;

    // reset seen status of mobs
    for (auto& p : this->mobs)
        p.second.seen = false;

    for (int i = 0; i < worldState->mobs()->size(); i++) {
        auto mobData = worldState->mobs()->Get(i);
        auto mobItr = this->mobs.find(mobData->id());
        if (mobItr == this->mobs.end()) {
            // this is the first time we see this mob, create it
            Mob newMob;
            newMob.sprite = CreateNewAnimSprite(this->cameraNode.get(), mobData->species());
            this->mobs[mobData->id()] = std::move(newMob);
            mobItr = this->mobs.find(mobData->id());
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
        if (mobItr->first == gameData.myID) {
            this->mySprite = mob.sprite.get();
        }
    }
    std::vector<int> deletedIDs;
    for (auto& mob : this->mobs) {
        if (!mob.second.seen)
            deletedIDs.push_back(mob.first);
    }
    for (auto id : deletedIDs) {
        this->mobs.erase(id);
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
    if (!this->mySprite)
        return;
    const float screenWidth = ncine::theApplication().width();
    const float screenHeight = ncine::theApplication().height();
    auto &mouseState = ncine::theApplication().inputManager().mouseState();
    auto myMobPosition = -this->mySprite->position() +
        ncine::Vector2f(3*screenWidth/4 - mouseState.x/2,
                        screenHeight/4 + mouseState.y/2);
    this->cameraNode->setPosition(myMobPosition);
}

void GameplayState::CleanUp() {

}

void GameplayState::OnMouseMoved(const ncine::MouseState &state) {
    // this->cameraNode->setPosition(ncine::Vector2f(state.x, state.y));
}