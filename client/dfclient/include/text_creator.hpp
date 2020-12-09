#pragma once

#include <ncine/Sprite.h>
#include "resources.hpp"
#include "gameplay_state.hpp"

/** Simplifies creating various in-game texts, such as UI messages on player kills.
 * Designed to allow long-term and one-off text parameter customization. As such,
 * stores the default values, which are customizable directly or through setter methods
 * and are supplied if no value for a parameter is given.
 * 
 * To simplify creating outlines, this manager also stores the values used to create
 * the last text. The values are supplied automatically to the outline-creating method,
 * with the exception of tween easing, which has to be given once more as a parameter.
 */
class TextCreator {
	std::reference_wrapper<Resources> _resources; // to be able to use fonts and emplace tweens
public:
	inline TextCreator(Resources& resources) : _resources(resources) {};

	// public for convenience
	ncine::Color _textColor= ncine::Color(0, 0, 0);
	float _pos_x = 0;
	float _pos_y = 0;
	float _textScale = 1.0f;
	int _from = 255, _hold = 60, _decay = 60;
	Layers _layer = Layers::TEXT;

private:
	std::string _lastText;
	ncine::Color _lastColor;
	float _last_x;
	float _last_y;
	float _lastScale;
	int _last_from, _last_hold, _last_decay;

public:
	// existent for code clarity
	inline void setColor(ncine::Color color) { _textColor = color; }
	inline void setColor(int r, int g, int b, int a = 255) { _textColor = ncine::Color(r,g,b,a); }

	inline void setPosition(float x, float y) { _pos_x = x; _pos_y = y; }
	inline void setScale(float scale) { _textScale = scale; }
	inline void setFrom(int from) { _from = from; }
	inline void setHold(int hold) { _hold = hold; }
	inline void setDecay(int decay) { _decay = decay; }
	inline void setTweenParams(int from, int hold, int decay) { setFrom(from); setHold(hold); setDecay(decay); }
	inline void setLayer(Layers layer) { _layer = layer; }

	/** Create an ncine::TextNode and (by default, but optionally) an alpha fadeout tween. 
	 * 
	 * The main function of the TextCreator. Returns a unique pointer to 
	 * a newly created ncine::TextNode and, unless stated otherwise via 
	 * parameters, creates an alpha fadeout tween, placing it in 
	 * _resources->_intTweens. 
	 * 
	 * The alpha fadeout tween starts at {from}, holds the initial value 
	 * for {hold} frames and then goes down to 0 during {decay} frames, 
	 * optionally following a function defined by {easing}. Specifying 
	 * {decay} as < 0 skips the tween creation, making the text permanently 
	 * visible unless later modified. 
	 * 
	 * Every parameter (besides text) can be left unspecified via omission or 
	 * std::nullopt to use previously set defaults. 
	 * 
	 * Texts created are scaled with regard to window width.
	 * 
	 * @param text The displayed text value. The only mandatory argument.
	 * @param color The color of the text. Not including alpha, which is set 
	 * 	via <from>.
	 * @param pos_x The x position of the text center, measured in pixels.
	 * @param pos_y The y posiion of the text center, measured in pixels.
	 * @param scale Scaling factor (1.0f is default value).
	 * @param from The starting alpha value (0-255).
	 * @param hold The number of frames to hold the initial alpha value.
	 * @param decay The number of frames to take to reduce alpha to 0. Set 
	 * 	as < 0 to skip tween creation.
	 * @param layer A layer from Layers enum to draw the text on.
	 * @param easing The tween easing function. For more details, see 
	 * 	the tweeny::via() definition.
	 * @return An std::unique_ptr< ncine::TextNode > of the created text.
	 */
	template <typename T = tweeny::easing::linearEasing>
	std::unique_ptr<ncine::TextNode> CreateText(
		std::string text,
		std::optional<ncine::Color> color = std::nullopt,
		std::optional<float> pos_x = std::nullopt,
		std::optional<float> pos_y = std::nullopt,
		std::optional<float> scale = std::nullopt,
		std::optional<int> from = std::nullopt,
		std::optional<int> hold = std::nullopt,
		std::optional<int> decay = std::nullopt,
		std::optional<Layers> layer = std::nullopt,
		const T& easing = tweeny::easing::linear
		);

	/** Create an outline of the last text. 
	 * 
	 * Creates an ncine::TextNode using the saved parameters of the most recently 
	 * created text and the outline font. The only parameter that must be additionally 
	 * specified (if not default) is the easing.
	 * 
	 * @param easing The easing used in the original text's tween.
	 * @return An std::unique_ptr< ncine::TextNode > of the text outline.
	 */
	template <typename T = tweeny::easing::linearEasing>
	std::unique_ptr<ncine::TextNode> CreateOutline(const T& easing = tweeny::easing::linear);

	template <typename T = tweeny::easing::linearEasing>
	tweeny::tween<int> CreateTextTween(ncine::TextNode* textPtr, int from = 255, int hold = 60, int decay = 60, const T& easing = tweeny::easing::linear);

private:
	template <typename T>
	std::unique_ptr<ncine::TextNode> doCreateText(std::string text, ncine::Color color,
		float pos_x, float pos_y, float scale, int from, int hold, int decay, Layers layer, const T& easing, bool isOutline);
};

#include "text_creator_templates.hpp"
