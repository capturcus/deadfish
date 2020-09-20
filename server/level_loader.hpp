#pragma once

void loadLevel(std::string& path);
flatbuffers::Offset<FlatBuffGenerated::Level> serializeLevel(flatbuffers::FlatBufferBuilder& builder);