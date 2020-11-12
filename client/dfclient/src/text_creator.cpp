#include "text_creator.hpp"

#include "gameplay_state.hpp"

TextCreator::TextCreator(Resources& resources) : _resources(resources) {
    _textColor = ncine::Color(0, 0, 0);
    _pos_x = 0;
    _pos_y = 0;
    _textScale = 1.0f;
    _from = 255; _hold = 60; _decay = 60;
    _layer = Layers::TEXT;
}
