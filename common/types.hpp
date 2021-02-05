#pragma once

template<typename T>
using MovableMap = std::map<uint16_t, std::unique_ptr<T>>;
