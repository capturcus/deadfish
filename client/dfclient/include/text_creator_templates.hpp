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
			easing,
			false);
	}

template <typename T>
std::unique_ptr<ncine::TextNode> TextCreator::CreateOutline(const T& easing) {
	if (_lastColor == ncine::Color::Black) return nullptr;
	return doCreateText(
		_lastText,
		ncine::Color::Black,
		_last_x,
		_last_y,
		_lastScale,
		_last_from,
		_last_hold,
		_last_decay,
		Layers::TEXT_OUTLINES,
		easing,
		true
	);
}


template <typename T>
std::unique_ptr<ncine::TextNode> TextCreator::doCreateText(std::string text, ncine::Color color,
        float pos_x, float pos_y, float scale, int from, int hold, int decay, Layers layer, const T& easing, bool isOutline) {
	auto screenWidth = ncine::theApplication().width();
	std::string fontName = isOutline ? "comic_outline" : "comic";
	auto ret = std::make_unique<ncine::TextNode>(&ncine::theApplication().rootNode(), _resources.get().fonts[fontName].get());
	ret->setString(text.c_str());
	ret->setColor(color);
	ret->setAlpha(from);
	ret->setPosition(pos_x, pos_y);
	ret->setScale(scale * 0.5f * screenWidth/1800.f); // font upgrade compensation and window size scaling
	ret->setLayer((unsigned short int)layer);
	if (decay >= 0) {	// decay < 0 makes the text permanent
		_resources.get()._intTweens.push_back(CreateTextTween(ret.get(), from, hold, decay, easing));
	}

	if (!isOutline) {
		_lastText = text;
		_lastColor = color;
		_last_x = pos_x;
		_last_y = pos_y;
		_lastScale = scale;
		_last_from = from;
		_last_hold = hold;
		_last_decay = decay;
	}
	return std::move(ret);
}
