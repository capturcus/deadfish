#!/usr/bin/env python3
import configparser, os, json, flatbuffers
import DeadFish.Bush, DeadFish.Level, DeadFish.Stone, DeadFish.Vec2

config = configparser.ConfigParser()
config.read("levelpacker.ini")

levels_dir = config['default']['levels_dir']

def process_level(path):
    with open(path) as f:
        tmxjson = json.loads(f.read())
    bushes = []
    stones = []
    builder = flatbuffers.Builder(1)
    for o in tmxjson["layers"][0]["objects"]:
        obj = {
            "x": o["x"]+o["width"]/2,
            "y": o["y"]-o["height"]/2,
            "radius": o["width"]/2
        }
        if o["gid"] == 1:
            DeadFish.Bush.BushStart(builder)
            DeadFish.Bush.BushAddRadius(builder, obj["radius"]*0.01)
            pos = DeadFish.Vec2.CreateVec2(builder, obj["x"]*0.01, obj["y"]*0.01)
            DeadFish.Bush.BushAddPos(builder, pos)
            bushes.append(DeadFish.Bush.BushEnd(builder))
        if o["gid"] == 2:
            DeadFish.Stone.StoneStart(builder)
            DeadFish.Stone.StoneAddRadius(builder, obj["radius"]*0.01)
            pos = DeadFish.Vec2.CreateVec2(builder, obj["x"]*0.01, obj["y"]*0.01)
            DeadFish.Stone.StoneAddPos(builder, pos)
            stones.append(DeadFish.Stone.StoneEnd(builder))
    
    DeadFish.Level.LevelStartBushesVector(builder, len(bushes))
    for b in bushes:
        builder.PrependUOffsetTRelative(b)
    bushesOff = builder.EndVector(len(bushes))

    DeadFish.Level.LevelStartStonesVector(builder, len(stones))
    for b in stones:
        builder.PrependUOffsetTRelative(b)
    stonesOff = builder.EndVector(len(stones))

    DeadFish.Level.LevelStartNavpointsVector(builder, 0)
    navpoints = builder.EndVector(0)

    DeadFish.Level.LevelStartNpcspawnsVector(builder, 0)
    npcspawns = builder.EndVector(0)

    DeadFish.Level.LevelStartPlayerpointsVector(builder, 0)
    playerpoints = builder.EndVector(0)

    DeadFish.Level.LevelStart(builder)
    DeadFish.Level.LevelAddBushes(builder, bushesOff)
    DeadFish.Level.LevelAddNavpoints(builder, navpoints)
    DeadFish.Level.LevelAddNpcspawns(builder, npcspawns)
    DeadFish.Level.LevelAddPlayerpoints(builder, playerpoints)
    DeadFish.Level.LevelAddStones(builder, stonesOff)
    size = DeadFish.Vec2.CreateVec2(builder,
        tmxjson["width"]*tmxjson["tilewidth"]*0.01,
        tmxjson["height"]*tmxjson["tileheight"]*0.01)
    DeadFish.Level.LevelAddSize(builder, size)
    level = DeadFish.Level.LevelEnd(builder)
    builder.Finish(level)

    buf = builder.Output()

    with open(path[:-4]+"bin", "wb") as f:
        f.write(buf)

for i in os.listdir(levels_dir):
    if i.endswith(".json"):
        # level file
        process_level(levels_dir+i)