#include <functional>
#include <iostream>

#include <ncine/Application.h>
#include <ncine/FileSystem.h>
#include <ncine/MeshSprite.h>
#include <ncine/Sprite.h>
#include <ncine/Texture.h>

#include "loading_state.hpp"
#include "resources.hpp"

namespace nc = ncine;

inline long fileSize(const char* path) { return path == "" ? 0 : ncine::FileSystem::fileSize(path); }
inline long fileSize(std::string path) { return path == "" ? 0 : ncine::FileSystem::fileSize(path.c_str()); }

// iterate over files to load from dir, calculating the total size and registering file names
long registerStuff(std::string path, std::vector<std::string>& filenames) {
	long total = 0;
	auto dir = ncine::FileSystem::Directory(path.c_str());
	const char* file = dir.readNext();
	while(file) {
		auto absPath = path + "/" + std::string(file);
		if(ncine::FileSystem::isFile(absPath.c_str())) {
			total += fileSize(absPath);
			filenames.push_back(absPath);
		}
		file = dir.readNext();
	}
	dir.close();
	return total;
}

LoadingState::LoadingState(Resources& r) : _resources(r) {
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	auto res = nc::theApplication().appConfiguration().resolution;
	nc::theApplication().gfxDevice().setClearColor(nc::Colorf::Black);

	// init paths
	auto rootPath = std::string(ncine::theApplication().appConfiguration().dataPath().data());

	// register everything to calculate total size
	_total += registerStuff(rootPath + TEXTURES_PATH, _textureFilenames);
	_total += registerStuff(rootPath + SOUNDS_PATH, _soundFilenames);

	// load fonts (for loading text and further use) and loading graphics
	_resources.fonts["comic_outline"] = std::make_unique<ncine::Font>((rootPath + "fonts/comic_outline.fnt").c_str());
	_resources.fonts["comic"] = std::make_unique<ncine::Font>((rootPath + "fonts/comic.fnt").c_str());

	_resources.textures["fillup"] = std::make_unique<ncine::Texture>((rootPath + "loading/fillup.png").c_str());
	_resources.textures["fillup_outline"] = std::make_unique<ncine::Texture>((rootPath + "loading/outline.png").c_str());
	_resources.textures["blackpixel.png"] = std::make_unique<ncine::Texture>((rootPath + "textures/blackpixel.png").c_str());

	// compensate blackpixel loading
	_current += fileSize(rootPath + "textures/blackpixel.png");
	std::remove(_textureFilenames.begin(), _textureFilenames.end(), rootPath + "textures/blackpixel.png");
	_textureFilenames.pop_back();

	// init loading text
	_loadingText = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());
	_loadingText->setColor(ncine::Color::White);
	_loadingText->setPosition(res.x*0.5f, res.y*0.5f);
	_loadingText->setString("Loading... 0%");

	// init loading graphics
	_fillupSprite = std::make_unique<ncine::Sprite>(&rootNode, _resources.textures["fillup"].get());
	_fillupSprite->setPosition(res.x*0.5f, res.y*0.6f);
	_fillupSprite->setLayer(1);

	_curtain = std::make_unique<ncine::MeshSprite>(&rootNode, _resources.textures["blackpixel.png"].get());
	std::vector<ncine::Vector2f> vertices(4);	
	vertices[0].x = _fillupSprite->position().x - _fillupSprite->width()/2;
	vertices[0].y = _fillupSprite->position().y - _fillupSprite->height()/2;
	vertices[1].x = _fillupSprite->position().x + _fillupSprite->width()/2;
	vertices[1].y = _fillupSprite->position().y - _fillupSprite->height()/2;
	vertices[2].x = _fillupSprite->position().x - _fillupSprite->width()/2;
	vertices[2].y = _fillupSprite->position().y + _fillupSprite->height()/2;
	vertices[3].x = _fillupSprite->position().x + _fillupSprite->width()/2;
	vertices[3].y = _fillupSprite->position().y + _fillupSprite->height()/2;
	_curtain->createVerticesFromTexels(vertices.size(), vertices.data());
	_curtain->setPosition(0, 0);
	_curtain->setLayer(2);

	_outline = std::make_unique<ncine::Sprite>(&rootNode, _resources.textures["fillup_outline"].get());
	_outline->setPosition(res.x*0.5f, res.y*0.6f);
	_outline->setLayer(3);
}

StateType LoadingState::Update(Messages m) {
	// load a single file
	std::string path = "";
	if (!_textureFilenames.empty()){
		path = _textureFilenames.back();
		std::string filename = ncine::FileSystem::baseName(path.c_str()).data();
		_resources.textures[filename] = std::make_unique<ncine::Texture>(path.c_str());
		_textureFilenames.pop_back();

	} else if (!_soundFilenames.empty()) {
		path = _soundFilenames.back();
		std::string filename = ncine::FileSystem::baseName(path.c_str()).data();
		_resources._sounds[filename] = std::make_unique<ncine::AudioBuffer>(path.c_str());
		_soundFilenames.pop_back();
	}
	_current += fileSize(path);

	// update the loading text
	auto _percent_loaded = _current/(double)_total * 100;
	auto string = "Loading... " + std::to_string((int)_percent_loaded) + "%";
	_loadingText->setString(string.c_str());
	unsigned char alpha = 55 + 2*_percent_loaded;
	_loadingText->setAlpha(alpha);

	// update the loading graphics
	_curtain->setPosition(0, 1.2*_percent_loaded);

	// manage the ending
	if (_percent_loaded == 100) {
		if (_fadeoutTween == nullptr) {
			// stretch curtain to cover the whole screen
			ncine::Vector2f res = {ncine::theApplication().width(), ncine::theApplication().height()};
			std::vector<ncine::Vector2f> vertices(4);	
			vertices[0].x = 0;
			vertices[0].y = -1.2*_percent_loaded;
			vertices[1].x = 0;
			vertices[1].y = res.y + 1 - 1.2*_percent_loaded;
			vertices[2].x = res.x + 1;
			vertices[2].y = -1.2*_percent_loaded;
			vertices[3].x = res.x + 1;
			vertices[3].y = res.y + 1 - 1.2*_percent_loaded;
			_curtain->createVerticesFromTexels(vertices.size(), vertices.data());
			_curtain->setPosition(0, 0);
			_curtain->setLayer(65535);
			_curtain->setAlpha(0);

			// create fadeout tween
			ncine::MeshSprite* curtainPtr = _curtain.get();
			auto fadeoutTween = tweeny::from(0).to(255).during(25).via(tweeny::easing::circularOut)
				.onStep([curtainPtr] (tweeny::tween<int>& t, int v) -> bool {
					curtainPtr->setAlpha(v);
					return false;
				});
			_resources._intTweens.push_back(std::move(fadeoutTween));
			_fadeoutTween = &(*(_resources._intTweens.rbegin()));  // ... -_-

		} else if (_fadeoutTween->progress() == 1) {
			return StateType::Menu;
		} 
	}
	return StateType::Loading;
}

LoadingState::~LoadingState() {
	// init soundbuffers
	_resources._killSoundBuffer = std::move(_resources._sounds["amongus-kill.wav"]);
	_resources._sounds.erase("amongus-kill.wav");
	_resources._killSound = std::make_unique<ncine::AudioBufferPlayer>(_resources._killSoundBuffer.get());

	_resources._goldfishSoundBuffer = std::move(_resources._sounds["mario-powerup.wav"]);
	_resources._sounds.erase("mario-powerup.wav");
	_resources._goldfishSound = std::make_unique<ncine::AudioBufferPlayer>(_resources._goldfishSoundBuffer.get());

	_curtain.reset();
}
