import * as Generated from '../deadfish_generated';
import { flatbuffers } from 'flatbuffers';

export namespace FBUtil {

    class GameData {
        level: any;
        initMeta: any;
        highscores: any = [];
        myName: string;
        gameRunning: boolean = true;
    }

    export let gameData: GameData = new GameData();

    export const MakeKillCommand = (id): Uint8Array => {
        let builder = new flatbuffers.Builder(0);
        let cmd = Generated.DeadFish.CommandKill.createCommandKill(builder, id);

        let message = Generated.DeadFish.ClientMessage.createClientMessage(
            builder,
            Generated.DeadFish.ClientMessageUnion.CommandKill,
            cmd
            );

        builder.finish(message);

        return builder.asUint8Array();
    }

    export const MakePlayerReady = (): Uint8Array => {
        let builder = new flatbuffers.Builder(0);
        let cmd = Generated.DeadFish.PlayerReady.createPlayerReady(builder);

        let message = Generated.DeadFish.ClientMessage.createClientMessage(
            builder,
            Generated.DeadFish.ClientMessageUnion.PlayerReady,
            cmd
            );

        builder.finish(message);

        return builder.asUint8Array();
    }

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
                // console.log("wrong type not simple", serverMsg.eventType());
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

    export const ParseDeathReport = (b: Uint8Array): Generated.DeadFish.DeathReport => {
        let buffer = new flatbuffers.ByteBuffer(b);
        let serverMsg = Generated.DeadFish.ServerMessage.getRootAsServerMessage(buffer);
        switch (serverMsg.eventType()) {
            case Generated.DeadFish.ServerMessageUnion.DeathReport:
                {
                    let ev = serverMsg.event(new Generated.DeadFish.DeathReport());
                    return ev;
                }
                break;
            default:
                return null;
        }
    }

    export const ParseHighscoreUpdate = (b: Uint8Array): Generated.DeadFish.HighscoreUpdate => {
        let buffer = new flatbuffers.ByteBuffer(b);
        let serverMsg = Generated.DeadFish.ServerMessage.getRootAsServerMessage(buffer);
        switch (serverMsg.eventType()) {
            case Generated.DeadFish.ServerMessageUnion.HighscoreUpdate:
                {
                    let ev = serverMsg.event(new Generated.DeadFish.HighscoreUpdate());
                    return ev;
                }
                break;
            default:
                // console.log("wrong type not simple", serverMsg.eventType());
                return null;
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
                // console.log("wrong type not simple", serverMsg.eventType());
                return null;
        }
    }

    export const ParseLevel = (b: Uint8Array): boolean => {
        let buffer = new flatbuffers.ByteBuffer(b);
        let serverMsg = Generated.DeadFish.ServerMessage.getRootAsServerMessage(buffer);
        switch (serverMsg.eventType()) {
            case Generated.DeadFish.ServerMessageUnion.Level:
                {
                    let level = {
                        bushes: [],
                        stones: []
                    };
                    let srvLevel = serverMsg.event(new Generated.DeadFish.Level());
                    for (let i = 0; i < srvLevel.bushesLength(); i++) {
                        let pos = srvLevel.bushes(i).pos();
                        level.bushes.push({
                            x: pos.x()*100,
                            y: pos.y()*100,
                            radius: srvLevel.bushes(i).radius()*100
                        });
                    }
                    for (let i = 0; i < srvLevel.stonesLength(); i++) {
                        let pos = srvLevel.stones(i).pos();
                        level.stones.push({
                            x: pos.x()*100,
                            y: pos.y()*100,
                            radius: srvLevel.stones(i).radius()*100
                        });
                    }
                    gameData.level = level;
                    return true;
                }
                break;
            default:
                console.log("wrong type not level", serverMsg.eventType());
                return false;
        }
    }
}