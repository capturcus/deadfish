#pragma once
#include "../../../common/deadfish_generated.h"

#define FBUtilGetServerEvent(DATA, TYPE) GetServerEvent<FlatBuffGenerated::TYPE>(DATA, FlatBuffGenerated::ServerMessageUnion_##TYPE)

template<typename T>
const T *GetServerEvent(const std::string& data, FlatBuffGenerated::ServerMessageUnion type) {
    auto serverMessage = flatbuffers::GetRoot<FlatBuffGenerated::ServerMessage>(data.data());
    if (serverMessage->event_type() != type)
        return nullptr;
    return (const T *)serverMessage->event();
}