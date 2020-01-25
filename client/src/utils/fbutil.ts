import * as Generated from '../deadfish_generated';
import { flatbuffers } from 'flatbuffers';

export namespace FBUtil {

    export let gameData: any = {
    };

    export const MakeJoinRequest = (name: string): Uint8Array => {
        let builder = new flatbuffers.Builder(0);
        let joinRequest = Generated.DeadFish.JoinRequest.createJoinRequest(builder, builder.createString(name));

        let message = Generated.DeadFish.ClientMessage.createClientMessage(
            builder,
            Generated.DeadFish.ClientMessageUnion.JoinRequest,
            joinRequest
            );

        builder.finish(message);

        return builder.asUint8Array();
    }

    export const ParseSimpleServerEvent = (b: Uint8Array): Generated.DeadFish.SimpleServerEventType => {
        let buffer = new flatbuffers.ByteBuffer(b);
        let serverMsg = Generated.DeadFish.ServerMessage.getRootAsServerMessage(buffer);
        switch (serverMsg.eventType()) {
            case Generated.DeadFish.ServerMessageUnion.SimpleServerEvent:
                {
                    console.log("is simple server event");
                    let ev = serverMsg.event(new Generated.DeadFish.SimpleServerEvent());
                    console.log("type", ev.type());
                    return ev.type();
                }
                break;
            default:
                console.log("wrong type not simple", serverMsg.eventType());
                return null;
        }
    }

    export const ParseInitMetadata = (b: Uint8Array): number => {
        let buffer = new flatbuffers.ByteBuffer(b);
        let serverMsg = Generated.DeadFish.ServerMessage.getRootAsServerMessage(buffer);
        switch (serverMsg.eventType()) {
            case Generated.DeadFish.ServerMessageUnion.InitMetadata:
                {
                    let ev = serverMsg.event(new Generated.DeadFish.InitMetadata());
                    gameData.initMeta = {
                        my_id: ev.yourid(),
                        level_id: ev.levelId(),
                        players: []
                    };
                    for (let i = 0; i<ev.playersLength(); i++) {
                        let player = ev.players(i);
                        gameData.initMeta.players.push({
                            name: player.name(),
                            id: player.id(),
                            species:player.species()
                        });
                    }
                    console.log("my id:", gameData.initMeta.my_id);
                    return 0;
                }
                break;
        
            default:
                // console.log("wrong type not init meta", serverMsg.eventType());
                return 1;
        }
    }

    export const ParseWorldState = (b: Uint8Array): Generated.DeadFish.WorldState => {
        let buffer = new flatbuffers.ByteBuffer(b);
        let serverMsg = Generated.DeadFish.ServerMessage.getRootAsServerMessage(buffer);
        switch (serverMsg.eventType()) {
            case Generated.DeadFish.ServerMessageUnion.WorldState:
                {
                    let ev = serverMsg.event(new Generated.DeadFish.WorldState());
                    return ev;
                }
                break;
            default:
                console.log("wrong type not simple", serverMsg.eventType());
                return null;
        }
    }
}