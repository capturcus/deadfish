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

LoadingState::LoadingState(Resources& r) : _resources(r) {
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	auto res = nc::theApplication().appConfiguration().resolution;
	nc::theApplication().gfxDevice().setClearColor(nc::Colorf::Black);

	// iterate over files to load, calculating the total size and registering file names
	// init paths
	auto rootPath = std::string(ncine::theApplication().appConfiguration().dataPath().data());
	auto textureDir = ncine::FileSystem::Directory((rootPath + std::string(TEXTURES_PATH)).c_str());
	auto soundDir = ncine::FileSystem::Directory((rootPath + SOUNDS_PATH).data());

	// register everything
	const char* file = textureDir.readNext();
	while (file)
	{
		auto absPath = rootPath + std::string(TEXTURES_PATH) + "/" + std::string(file);
		if (ncine::FileSystem::isFile(absPath.c_str())) {
			_total += fileSize(absPath);
			_textureFilenames.push_back(absPath);
		}
		file = textureDir.readNext();
	}
	textureDir.close();

	file = soundDir.readNext();
	while (file) {
		auto absPath = rootPath.data() + std::string(SOUNDS_PATH) + "/" + std::string(file);
		if (ncine::FileSystem::isFile(absPath.c_str())) {
			_total += fileSize(absPath);
			_soundFilenames.push_back(absPath);
		}
		file = soundDir.readNext();
	}

	// load fonts (for loading text and further use) and loading graphics
	_resources.fonts["comic_outline"] = std::make_unique<ncine::Font>((rootPath + "fonts/comic_outline.fnt").c_str());
	_resources.fonts["comic"] = std::make_unique<ncine::Font>((rootPath + "fonts/comic.fnt").c_str());

	_resources.textures["fillup"] = std::make_unique<ncine::Texture>((rootPath + "loading/fillup.png").c_str());
	_resources.textures["fillup_outline"] = std::make_unique<ncine::Texture>((rootPath + "loading/outline.png").c_str());
	_resources.textures["blackpixel.png"] = std::make_unique<ncine::Texture>((rootPath + "loading/blackpixel.png").c_str());

	// init loading text
	_loading_text = std::make_unique<ncine::TextNode>(&rootNode, _resources.fonts["comic"].get());
	_loading_text->setColor(ncine::Color::White);
	_loading_text->setPosition(res.x*0.5f, res.y*0.5f);
	_loading_text->setString("Loading... 0%");

	// init loading graphics
	_fillup_sprite = std::make_unique<ncine::Sprite>(&rootNode, _resources.textures["fillup"].get());
	_fillup_sprite->setPosition(res.x*0.5f, res.y*0.6f);
	_fillup_sprite->setLayer(1);

	_curtain = std::make_unique<ncine::MeshSprite>(&rootNode, _resources.textures["blackpixel.png"].get());
	std::vector<ncine::Vector2f> vertices(4);	
	vertices[0].x = _fillup_sprite->position().x - _fillup_sprite->width()/2;
	vertices[0].y = _fillup_sprite->position().y - _fillup_sprite->height()/2;
	vertices[1].x = _fillup_sprite->position().x + _fillup_sprite->width()/2;
	vertices[1].y = _fillup_sprite->position().y - _fillup_sprite->height()/2;
	vertices[2].x = _fillup_sprite->position().x - _fillup_sprite->width()/2;
	vertices[2].y = _fillup_sprite->position().y + _fillup_sprite->height()/2;
	vertices[3].x = _fillup_sprite->position().x + _fillup_sprite->width()/2;
	vertices[3].y = _fillup_sprite->position().y + _fillup_sprite->height()/2;
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
	_loading_text->setString(string.c_str());
	unsigned char alpha = 55 + 2*_percent_loaded;
	_loading_text->setAlpha(alpha);

	// update the loading graphics
	_curtain->setPosition(0, 1.2*_percent_loaded);

	// return
	if (_percent_loaded == 100) return StateType::Menu;
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

}
