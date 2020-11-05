#pragma once

#include <ncine/Sprite.h>
#include "resources.hpp"

class TextCreator {
    Resources& _resources;
public:
    TextCreator(Resources& resources);

    // public for convenience
    ncine::Color _textColor;
    float _pos_x;
    float _pos_y;
    float _textScale;
    int _from, _hold, _decay;

    // existent for code clarity
    inline void setColor(ncine::Color color) { _textColor = color; }
    inline void setColor(int r, int g, int b, int a = 255) { _textColor = ncine::Color(r,g,b,a); }

    inline void setPosition(float x, float y) { _pos_x = x; _pos_y = y; }
    inline void setScale(float scale) { _textScale = scale; }
    inline void setFrom(int from) { _from = from; }
    inline void setHold(int hold) { _hold = hold; }
    inline void setDecay(int decay) { _decay = decay; }
    inline void setTweenParams(int from, int hold, int decay) { setFrom(from); setHold(hold); setDecay(decay); }

    std::unique_ptr<ncine::TextNode> CreateText(
        std::string text,
        std::optional<ncine::Color> color = std::nullopt,
        std::optional<float> pos_x = std::nullopt,
        std::optional<float> pos_y = std::nullopt,
        std::optional<float> scale = std::nullopt,
        std::optional<int> from = std::nullopt,
        std::optional<int> hold = std::nullopt,
        std::optional<int> decay = std::nullopt
        );

private:
    std::unique_ptr<ncine::TextNode> doCreateText(std::string text, ncine::Color color,
        float pos_x, float pos_y, float scale, int from, int hold, int decay);
};

extern std::optional<TextCreator> textCreator;
