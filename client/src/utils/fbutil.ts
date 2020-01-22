import * as Generated from '../deadfish_generated';
import { flatbuffers } from 'flatbuffers';

export namespace FBUtil {
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

    export const ParseInitMetadata = (b: Uint8Array) => {
        console.log(b);
        let buffer = new flatbuffers.ByteBuffer(b);
        let serverMsg = Generated.DeadFish.ServerMessage.getRootAsServerMessage(buffer);
        switch (serverMsg.eventType()) {
            case Generated.DeadFish.ServerMessageUnion.InitMetadata:
                {
                    let ev = serverMsg.event(new Generated.DeadFish.InitMetadata());
                    console.log("my id", ev.yourid());
                    console.log("level id", ev.levelId());
                    console.log("players:");
                    for (let i = 0; i<ev.playersLength(); i++) {
                        let player = ev.players(i);
                        console.log("name ", player.name());
                        console.log("id ", player.id());
                        console.log("species", player.species());
                    }
                }
                break;
        
            default:
                console.log("wrong type", serverMsg.eventType());
                break;
        }
    }
}