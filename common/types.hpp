#pragma once

#include <map>
#include <memory>
#include <vector>
#include <ncine/DrawableNode.h>

template<typename T>
using MovableMap = std::map<uint16_t, std::unique_ptr<T>>;

typedef std::vector<std::unique_ptr<ncine::DrawableNode>> DrawableNodeVector;
