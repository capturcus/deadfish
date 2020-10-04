#!/usr/bin/env python3
import configparser, os, json, flatbuffers
import FlatBuffGenerated.Level, FlatBuffGenerated.Visible, FlatBuffGenerated.HidingSpot, \
    FlatBuffGenerated.Collision, FlatBuffGenerated.Vec2, \
    FlatBuffGenerated.Tile, FlatBuffGenerated.Tileset, FlatBuffGenerated.TileArray, \
    FlatBuffGenerated.NavPoint, FlatBuffGenerated.PlayerWall
from functools import reduce
import xml.dom.minidom as minidom
from collections import namedtuple


GLOBAL_SCALE = 0.01


config = configparser.ConfigParser()
config.read("levelpacker.ini")

levels_dir = config['default']['levels_dir']

GameObjects = namedtuple('GameObjects', ['visible', 'collision', 'hidingspots', 'navpoints', 'playerwalls', 'tilesets'])


def get_pos(o: minidom.Node, flip_y: bool = False) -> (float, float):
    x = float(o.getAttribute('x')) + float(o.getAttribute('width') or 0) / 2.0
    y = float(o.getAttribute('y'))
    height = float(o.getAttribute('height') or 0) / 2.0
    y += -height if flip_y else height
    return x, y


def handle_visible_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y = get_pos(o, flip_y=True)
        # radius = float(o.getAttribute('width')) / 2.0
        rot = float(o.getAttribute('rotation') or 0)
        gid = int(o.getAttribute('gid'))

        FlatBuffGenerated.Visible.VisibleStart(builder)
        pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
        FlatBuffGenerated.Visible.VisibleAddPos(builder, pos)
        FlatBuffGenerated.Visible.VisibleAddRotation(builder, rot)
        FlatBuffGenerated.Visible.VisibleAddGid(builder, gid)
        objs.visible.append(FlatBuffGenerated.Visible.VisibleEnd(builder))

def handle_collision_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y = get_pos(o)
        ellipse = False
        radius = 0
        polyverts = ""
        polyvertsOffset = []

        if o.getElementsByTagName('ellipse'):
            ellipse = True
            radius = float(o.getAttribute('width')) / 2.0
        else:
            polyverts = o.getElementsByTagName('polygon')[0].getAttribute('points')
            polyverts = polyverts.split(' ')
            for v in polyverts:
                vert_x, vert_y = v.split(",")
                vert_x_f = float(vert_x)
                vert_y_f = float(vert_y)
                vec = FlatBuffGenerated.Vec2.CreateVec2(builder, vert_x_f, vert_y_f)
                polyvertsOffset.append(vec)
        
        FlatBuffGenerated.Collision.CollisionStartPolyvertsVector(builder, len(polyvertsOffset))
        for v in polyvertsOffset:
            builder.PrependUOffsetTRelative(v)
        poly = builder.EndVector(len(polyvertsOffset))

        FlatBuffGenerated.Collision.CollisionStart(builder)
        pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
        FlatBuffGenerated.Collision.CollisionAddPos(builder, pos)
        FlatBuffGenerated.Collision.CollisionAddEllipse(builder, ellipse)
        FlatBuffGenerated.Collision.CollisionAddRadius(builder, radius)
        FlatBuffGenerated.Collision.CollisionAddPolyverts(builder, poly)
        objs.hidingspots.append(FlatBuffGenerated.Collision.CollisionEnd(builder))


def handle_hidingspots_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y = get_pos(o)
        ellipse = False
        radius = 0
        polyverts = ""
        polyvertsOffset = []

        if o.getElementsByTagName('ellipse'):
            ellipse = True
            radius = float(o.getAttribute('width')) / 2.0
        else:
            polyverts = o.getElementsByTagName('polygon')[0].getAttribute('points')
            polyverts = polyverts.split(' ')
            for v in polyverts:
                vert_x, vert_y = v.split(",")
                vert_x_f = float(vert_x)
                vert_y_f = float(vert_y)
                vec = FlatBuffGenerated.Vec2.CreateVec2(builder, vert_x_f, vert_y_f)
                polyvertsOffset.append(vec)
        
        FlatBuffGenerated.HidingSpot.HidingSpotStartPolyvertsVector(builder, len(polyvertsOffset))
        for v in polyvertsOffset:
            builder.PrependUOffsetTRelative(v)
        poly = builder.EndVector(len(polyvertsOffset))

        FlatBuffGenerated.HidingSpot.HidingSpotStart(builder)
        pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
        FlatBuffGenerated.HidingSpot.HidingSpotAddPos(builder, pos)
        FlatBuffGenerated.HidingSpot.HidingSpotAddEllipse(builder, ellipse)
        FlatBuffGenerated.HidingSpot.HidingSpotAddRadius(builder, radius)
        FlatBuffGenerated.HidingSpot.HidingSpotAddPolyverts(builder, poly)
        objs.hidingspots.append(FlatBuffGenerated.HidingSpot.HidingSpotEnd(builder))


def handle_meta_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y = get_pos(o)

        typ = o.getAttribute('type')

        if typ == 'playerwall':
            width = float(o.getAttribute('width')) / 2.0
            height = float(o.getAttribute('height')) / 2.0

            FlatBuffGenerated.PlayerWall.PlayerWallStart(builder)
            pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
            FlatBuffGenerated.PlayerWall.PlayerWallAddPosition(builder, pos)
            size = FlatBuffGenerated.Vec2.CreateVec2(builder, width * GLOBAL_SCALE, height * GLOBAL_SCALE)
            FlatBuffGenerated.PlayerWall.PlayerWallAddSize(builder, size)
            pwall = FlatBuffGenerated.PlayerWall.PlayerWallEnd(builder)
            objs.playerwalls.append(pwall)

        elif typ == 'waypoint':
            name = o.getAttribute('name')

            neighbors = []
            isspawn = False
            isplayerspawn = False

            width = float(o.getAttribute('width'))
            height = float(o.getAttribute('height'))
            if width != height:
                print("waypoint", name, "is an ellipse, not a circle, width:", width, "height:", height)
                exit(1)
            radius = (width / 2.0) * GLOBAL_SCALE

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
            FlatBuffGenerated.NavPoint.NavPointStartNeighborsVector(builder, len(neighOff))
            for b in neighOff:
                builder.PrependUOffsetTRelative(b)
            neighs = builder.EndVector(len(neighOff))
            FlatBuffGenerated.NavPoint.NavPointStart(builder)
            pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
            FlatBuffGenerated.NavPoint.NavPointAddPosition(builder, pos)
            FlatBuffGenerated.NavPoint.NavPointAddRadius(builder, radius)
            FlatBuffGenerated.NavPoint.NavPointAddName(builder, name)
            FlatBuffGenerated.NavPoint.NavPointAddNeighbors(builder, neighs)
            FlatBuffGenerated.NavPoint.NavPointAddIsspawn(builder, isspawn)
            FlatBuffGenerated.NavPoint.NavPointAddIsplayerspawn(builder, isplayerspawn)
            objs.navpoints.append(FlatBuffGenerated.NavPoint.NavPointEnd(builder))


def handle_tileset(ts: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    path = builder.CreateString(ts.getAttribute('source'))
    firstgid = int(ts.getAttribute('firstgid'))
    FlatBuffGenerated.Tileset.TilesetStart(builder)
    FlatBuffGenerated.Tileset.TilesetAddPath(builder, path)
    FlatBuffGenerated.Tileset.TilesetAddFirstgid(builder, firstgid)
    objs.tilesets.append(FlatBuffGenerated.Tileset.TilesetEnd(builder))


def process_level(path: str):
    dom = minidom.parse(path)
    map_node = dom.firstChild
    objs = GameObjects([], [], [], [], [], [])
    builder = flatbuffers.Builder(1)

    for ts in map_node.getElementsByTagName('tileset'):
        handle_tileset(ts, objs, builder)

    for g in map_node.getElementsByTagName('objectgroup'):
        group_name = g.getAttribute('name')
        if group_name == 'visible':
            handle_visible_layer(g, objs, builder)
        elif group_name == 'collision':
            handle_collision_layer(g, objs, builder)
        elif group_name == 'hidingspots':
            handle_hidingspots_layer(g, objs, builder)
        elif group_name == 'meta':
            handle_meta_layer(g, objs, builder)
        else:
            print("WARNING: Unknown group {}".format(group_name))
    
    FlatBuffGenerated.Level.LevelStartVisibleVector(builder, len(objs.visible))
    for v in objs.visible:
        builder.PrependUOffsetTRelative(v)
    visibleOff = builder.EndVector(len(objs.visible))

    FlatBuffGenerated.Level.LevelStartCollisionVector(builder, len(objs.collision))
    for c in objs.collision:
        builder.PrependUOffsetTRelative(c)
    collisionOff = builder.EndVector(len(objs.collision))

    FlatBuffGenerated.Level.LevelStartHidingspotsVector(builder, len(objs.hidingspots))
    for hs in objs.hidingspots:
        builder.PrependUOffsetTRelative(hs)
    hspotsOff = builder.EndVector(len(objs.hidingspots))

    FlatBuffGenerated.Level.LevelStartNavpointsVector(builder, len(objs.navpoints))
    for b in objs.navpoints:
        builder.PrependUOffsetTRelative(b)
    navpointsOff = builder.EndVector(len(objs.navpoints))

    FlatBuffGenerated.Level.LevelStartPlayerwallsVector(builder, len(objs.playerwalls))
    for b in objs.playerwalls:
        builder.PrependUOffsetTRelative(b)
    playerwallsOff = builder.EndVector(len(objs.playerwalls))

    FlatBuffGenerated.Level.LevelStartTilesetsVector(builder, len(objs.tilesets))
    for t in objs.tilesets:
        builder.PrependUOffsetTRelative(t)
    tilesetsOff = builder.EndVector(len(objs.tilesets))

    FlatBuffGenerated.Level.LevelStart(builder)
    FlatBuffGenerated.Level.LevelAddVisible(builder, visibleOff)
    FlatBuffGenerated.Level.LevelAddCollision(builder, collisionOff)
    FlatBuffGenerated.Level.LevelAddHidingspots(builder, hspotsOff)
    FlatBuffGenerated.Level.LevelAddNavpoints(builder, navpointsOff)
    FlatBuffGenerated.Level.LevelAddPlayerwalls(builder, playerwallsOff)
    size = FlatBuffGenerated.Vec2.CreateVec2(builder,
        float(map_node.getAttribute("width")) * float(map_node.getAttribute("tilewidth")) * GLOBAL_SCALE,
        float(map_node.getAttribute("height")) * float(map_node.getAttribute("tileheight")) * GLOBAL_SCALE)
    FlatBuffGenerated.Level.LevelAddSize(builder, size)
    FlatBuffGenerated.Level.LevelAddTilesets(builder, tilesetsOff)
    level = FlatBuffGenerated.Level.LevelEnd(builder)
    builder.Finish(level)

    buf = builder.Output()

    with open(path[:-3]+"bin", "wb") as f:
        f.write(buf)


def process_tileset(path: str):
    dom = minidom.parse(path)
    tileset_node = dom.firstChild
    builder = flatbuffers.Builder(1)
    
    tiles = []
    
    for t in tileset_node.getElementsByTagName("tile"):
        id = int(t.getAttribute("id"))
        image = t.getElementsByTagName("image")[0]
        source = builder.CreateString(image.getAttribute("source"))

        FlatBuffGenerated.Tile.TileStart(builder)
        FlatBuffGenerated.Tile.TileAddPath(builder, source)
        size = FlatBuffGenerated.Vec2.CreateVec2(builder, float(image.getAttribute("width")), float(image.getAttribute("height")))
        FlatBuffGenerated.Tile.TileAddSize(builder, size)
        FlatBuffGenerated.Tile.TileAddId(builder, id)
        tiles.append(FlatBuffGenerated.Tile.TileEnd(builder))

    FlatBuffGenerated.TileArray.TileArrayStartTilesVector(builder, len(tiles))
    for t in tiles:
        builder.PrependUOffsetTRelative(t)
    tilesOff = builder.EndVector(len(tiles))

    FlatBuffGenerated.TileArray.TileArrayStart(builder)
    FlatBuffGenerated.TileArray.TileArrayAddTiles(builder, tilesOff)
    tileset = FlatBuffGenerated.TileArray.TileArrayEnd(builder)
    builder.Finish(tileset)

    buf = builder.Output()

    with open(path[:-3]+"tsbin", "wb") as f:
        f.write(buf)


# the actual script beginning
for i in os.listdir(levels_dir):
    if i.endswith(".tmx"):
        # level file
        process_level(levels_dir+i)
for ts in os.listdir(levels_dir+"tilesets"):
    if ts.endswith(".tsx"):
        # tileset file
        process_tileset(levels_dir+"tilesets/"+ts)