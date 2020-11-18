#include <ncine/Application.h>

// TEMPLATE DEFINITION(S)

template <typename T>
tweeny::tween<int> TextCreator::CreateTextTween(ncine::TextNode* textPtr, int from, int hold, int decay, const T& easing) {

	auto tween = tweeny::from(from)
		.to(from).during(hold)
		.to(0).during(decay).via(easing).onStep(
		[textPtr] (tweeny::tween<int>& t, int v) -> bool {
			auto textColor = textPtr->color();
			textPtr->setColor(textColor.r(), textColor.g(), textColor.b(), v);
			return false;
		}
	);
	return tween;
}

template <typename T>
std::unique_ptr<ncine::TextNode> TextCreator::CreateText(
	std::string text,
	std::optional<ncine::Color> color,
	std::optional<float> pos_x,
	std::optional<float> pos_y,
	std::optional<float> scale,
	std::optional<int> from,
	std::optional<int> hold,
	std::optional<int> decay,
	std::optional<Layers> layer,
	const T& easing) {
		return doCreateText(
			text,
			color.value_or(_textColor),
			pos_x.value_or(_pos_x),
			pos_y.value_or(_pos_y),
			scale.value_or(_textScale),
			from.value_or(_from),
			hold.value_or(_hold),
			decay.value_or(_decay),
			layer.value_or(_layer),
			easing);
	}

template <typename T>
std::unique_ptr<ncine::TextNode> TextCreator::doCreateText(std::string text, ncine::Color color,
        float pos_x, float pos_y, float scale, int from, int hold, int decay, Layers layer, const T& easing) {
	auto ret = std::make_unique<ncine::TextNode>(&ncine::theApplication().rootNode(), _resources.fonts["comic"].get());
	ret->setString(text.c_str());
	ret->setColor(color);
	ret->setAlpha(from);
	ret->setPosition(pos_x, pos_y);
	ret->setScale(scale*0.5f); // because the font was made 2x bigger on font_upgrade
	ret->setLayer((unsigned short int)layer);
	if (decay >= 0) {	// decay < 0 makes the text permanent
	_resources._intTweens.push_back(CreateTextTween(ret.get(), from, hold, decay, easing));
	}
	return std::move(ret);
}
