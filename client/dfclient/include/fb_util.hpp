#pragma once
#include "../../../common/deadfish_generated.h"

#define FBUtilGetServerEvent(DATA, TYPE) GetServerEvent<DeadFish::TYPE>(DATA, DeadFish::ServerMessageUnion_##TYPE)

template<typename T>
const T *GetServerEvent(std::string& data, DeadFish::ServerMessageUnion type) {
    auto serverMessage = flatbuffers::GetRoot<DeadFish::ServerMessage>(data.data());
    if (serverMessage->event_type() != type)
        return nullptr;
    return (const T *)serverMessage->event();
}