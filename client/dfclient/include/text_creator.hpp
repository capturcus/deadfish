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
    inline void setTextColor(ncine::Color color) { _textColor = color; }
    inline void setTextColor(int r, int g, int b, int a = 255) { _textColor = ncine::Color(r,g,b,a); }

    inline void setTextPosition(float x, float y) { _pos_x = x; _pos_y = y; }
    inline void setTextScale(float scale) { _textScale = scale; }
    inline void setFrom(int from) { _from = from; }
    inline void setHold(int hold) { _hold = hold; }
    inline void setDecay(int decay) { _decay = decay; }
    inline void setTweenParams(int from, int hold, int decay) { setFrom(from); setHold(hold); setDecay(decay); }

    // std::unique_ptr<ncine::TextNode> CreateText(std::string text);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, ncine::Color color);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, float pos_x, float pos_y);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, ncine::Color color, float pos_x, float pos_y);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, float scale);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, ncine::Color, float scale);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, ncine::Color, float pos_x, float pos_y, float scale);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, int from, int hold, int decay);
    // std::unique_ptr<ncine::TextNode> CreateText(std::string text, ncine::Color, int from, int hold, int decay);

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