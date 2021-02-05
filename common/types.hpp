#pragma once

#include <map>
#include <memory>
#include <vector>

template<typename T>
using MovableMap = std::map<uint16_t, std::unique_ptr<T>>;
