#include "text_creator.hpp"

#include <ncine/Application.h>

#include "gameplay_state.hpp"
#include "util.hpp"

TextCreator::TextCreator(Resources& resources) : _resources(resources) {
    _textColor = ncine::Color(0, 0, 0);
    _pos_x = 0;
    _pos_y = 0;
    _textScale = 1.0f;
    _from = 255; _hold = 60; _decay = 60;
}

std::unique_ptr<ncine::TextNode> TextCreator::CreateText(
	std::string text,
	std::optional<ncine::Color> color,
	std::optional<float> pos_x,
	std::optional<float> pos_y,
	std::optional<float> scale,
	std::optional<int> from,
	std::optional<int> hold,
	std::optional<int> decay) {
		return doCreateText(
			text,
			color.value_or(_textColor),
			pos_x.value_or(_pos_x),
			pos_y.value_or(_pos_y),
			scale.value_or(_textScale),
			from.value_or(_from),
			hold.value_or(_hold),
			decay.value_or(_decay));
	}


std::unique_ptr<ncine::TextNode> TextCreator::doCreateText(std::string text, ncine::Color color,
        float pos_x, float pos_y, float scale, int from, int hold, int decay) {
	auto ret = std::make_unique<ncine::TextNode>(&ncine::theApplication().rootNode(), _resources.fonts["comic"].get());
	ret->setString(text.c_str());
	ret->setColor(color);
	ret->setPosition(pos_x, pos_y);
	ret->setScale(scale);
	_resources._intTweens.push_back(CreateTextTween(ret.get(), from, hold, decay));
	return std::move(ret);
}
