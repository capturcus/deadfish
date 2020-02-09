#!/usr/bin/env python3
import configparser, os, json, flatbuffers
import DeadFish.Bush, DeadFish.Level, DeadFish.Stone, \
    DeadFish.Vec2, DeadFish.NavPoint, DeadFish.PlayerWall
from functools import reduce
import xml.dom.minidom as minidom
from collections import namedtuple


GLOBAL_SCALE = 0.01


config = configparser.ConfigParser()
config.read("levelpacker.ini")

levels_dir = config['default']['levels_dir']

GameObjects = namedtuple('GameObjects', ['bushes', 'stones', 'navpoints', 'playerwalls'])


def get_pos(o: minidom.Node, flip_y: bool = False) -> (float, float):
    x = float(o.getAttribute('x')) + float(o.getAttribute('width') or 0) / 2.0
    y = float(o.getAttribute('y'))
    height = float(o.getAttribute('height') or 0) / 2.0
    y += -height if flip_y else height
    return x, y


def handle_objects(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y = get_pos(o, flip_y=True)
        radius = float(o.getAttribute('width')) / 2.0

        gid = int(o.getAttribute('gid'))

        if gid == 1:
            DeadFish.Bush.BushStart(builder)
            DeadFish.Bush.BushAddRadius(builder, radius * GLOBAL_SCALE)
            pos = DeadFish.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
            DeadFish.Bush.BushAddPos(builder, pos)
            objs.bushes.append(DeadFish.Bush.BushEnd(builder))

        elif gid == 2:
            DeadFish.Stone.StoneStart(builder)
            DeadFish.Stone.StoneAddRadius(builder, radius * GLOBAL_SCALE)
            pos = DeadFish.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
            DeadFish.Stone.StoneAddPos(builder, pos)
            objs.stones.append(DeadFish.Stone.StoneEnd(builder))


def handle_meta(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y = get_pos(o)

        typ = o.getAttribute('type')

        if typ == 'playerwall':
            width = float(o.getAttribute('width')) / 2.0
            height = float(o.getAttribute('height')) / 2.0

            DeadFish.PlayerWall.PlayerWallStart(builder)
            pos = DeadFish.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
            DeadFish.PlayerWall.PlayerWallAddPosition(builder, pos)
            size = DeadFish.Vec2.CreateVec2(builder, width * GLOBAL_SCALE, height * GLOBAL_SCALE)
            DeadFish.PlayerWall.PlayerWallAddSize(builder, size)
            pwall = DeadFish.PlayerWall.PlayerWallEnd(builder)
            objs.playerwalls.append(pwall)

        elif typ == 'waypoint':
            name = o.getAttribute('name')

            neighbors = []
            isspawn = False
            isplayerspawn = False

            for prop in o.getElementsByTagName('property'):
                prop_name = prop.getAttribute('name')
                value = prop.getAttribute('value')
                if prop_name == 'wps':
                    neighbors = value.split(',')
                elif prop_name == 'isspawn':
                    isspawn = (value == 'true')
                elif prop_name == 'isplayerspawn':
                    isplayerspawn = (value == 'true')
                else:
                    print("WARNING: Unknown property {}".format(prop_name))

            name = builder.CreateString(name)
            neighOff = [builder.CreateString(x) for x in neighbors]
            DeadFish.NavPoint.NavPointStartNeighborsVector(builder, len(neighOff))
            for b in neighOff:
                builder.PrependUOffsetTRelative(b)
            neighs = builder.EndVector(len(neighOff))
            DeadFish.NavPoint.NavPointStart(builder)
            pos = DeadFish.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
            DeadFish.NavPoint.NavPointAddPosition(builder, pos)
            DeadFish.NavPoint.NavPointAddName(builder, name)
            DeadFish.NavPoint.NavPointAddNeighbors(builder, neighs)
            DeadFish.NavPoint.NavPointAddIsspawn(builder, isspawn)
            DeadFish.NavPoint.NavPointAddIsplayerspawn(builder, isplayerspawn)
            objs.navpoints.append(DeadFish.NavPoint.NavPointEnd(builder))


def process_level(path: str):
    dom = minidom.parse(path)
    map_node = dom.firstChild
    objs = GameObjects([], [], [], [])
    builder = flatbuffers.Builder(1)

    for g in map_node.getElementsByTagName('objectgroup'):
        group_name = g.getAttribute('name')
        if group_name == 'objects':
            handle_objects(g, objs, builder)
        elif group_name == 'meta':
            handle_meta(g, objs, builder)
        else:
            print("WARNING: Unknown group {}".format(group_name))
    
    DeadFish.Level.LevelStartBushesVector(builder, len(objs.bushes))
    for b in objs.bushes:
        builder.PrependUOffsetTRelative(b)
    bushesOff = builder.EndVector(len(objs.bushes))

    DeadFish.Level.LevelStartStonesVector(builder, len(objs.stones))
    for b in objs.stones:
        builder.PrependUOffsetTRelative(b)
    stonesOff = builder.EndVector(len(objs.stones))

    DeadFish.Level.LevelStartNavpointsVector(builder, len(objs.navpoints))
    for b in objs.navpoints:
        builder.PrependUOffsetTRelative(b)
    navpointsOff = builder.EndVector(len(objs.navpoints))

    DeadFish.Level.LevelStartPlayerwallsVector(builder, len(objs.playerwalls))
    for b in objs.playerwalls:
        builder.PrependUOffsetTRelative(b)
    playerwallsOff = builder.EndVector(len(objs.playerwalls))

    DeadFish.Level.LevelStart(builder)
    DeadFish.Level.LevelAddBushes(builder, bushesOff)
    DeadFish.Level.LevelAddStones(builder, stonesOff)
    DeadFish.Level.LevelAddNavpoints(builder, navpointsOff)
    DeadFish.Level.LevelAddPlayerwalls(builder, playerwallsOff)
    size = DeadFish.Vec2.CreateVec2(builder,
        float(map_node.getAttribute("width")) * float(map_node.getAttribute("tilewidth")) * GLOBAL_SCALE,
        float(map_node.getAttribute("height")) * float(map_node.getAttribute("tileheight")) * GLOBAL_SCALE)
    DeadFish.Level.LevelAddSize(builder, size)
    level = DeadFish.Level.LevelEnd(builder)
    builder.Finish(level)

    buf = builder.Output()

    with open(path[:-3]+"bin", "wb") as f:
        f.write(buf)


for i in os.listdir(levels_dir):
    if i.endswith(".tmx"):
        # level file
        process_level(levels_dir+i)