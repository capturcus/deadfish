#!/usr/bin/env python3
import configparser, os, json, flatbuffers, math
import FlatBuffGenerated.Level, FlatBuffGenerated.Object, FlatBuffGenerated.HidingSpot, \
    FlatBuffGenerated.Collision, FlatBuffGenerated.Decoration, FlatBuffGenerated.Vec2, \
    FlatBuffGenerated.Tileinfo, FlatBuffGenerated.NavPoint, FlatBuffGenerated.PlayerWall
from functools import reduce
import xml.dom.minidom as minidom
from collections import namedtuple


GLOBAL_SCALE = 0.01


config = configparser.ConfigParser()
config.read("levelpacker.ini")

levels_dir = config['default']['levels_dir']

GameObjects = namedtuple('GameObjects', ['objects', 'decoration', 'collision', 'hidingspots', 'navpoints', 'playerwalls', 'tileinfo'])


def get_pos(o: minidom.Node) -> (float, float, float):
    x = float(o.getAttribute('x'))
    y = float(o.getAttribute('y'))
    rotation = float(o.getAttribute('rotation') or 0)
    return x, y, rotation

def handle_objects_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y, rot = get_pos(o)
        gid = int(o.getAttribute('gid'))
        hspot = (o.getAttribute('type') == "hidingspot")
        hspotname = ""
        
        if hspot:
            for prop in o.getElementsByTagName("property"):
                if prop.getAttribute("name") == "hspotName":
                    hspotname = prop.getAttribute("value")
        hspotNameFb = builder.CreateString(hspotname)

        FlatBuffGenerated.Object.ObjectStart(builder)
        pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
        FlatBuffGenerated.Object.ObjectAddPos(builder, pos)
        FlatBuffGenerated.Object.ObjectAddRotation(builder, rot)
        FlatBuffGenerated.Object.ObjectAddGid(builder, gid)
        FlatBuffGenerated.Object.ObjectAddHspot(builder, hspot)
        FlatBuffGenerated.Object.ObjectAddHspotname(builder, hspotNameFb)
        objs.objects.append(FlatBuffGenerated.Object.ObjectEnd(builder))

def handle_decoration_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y, rot = get_pos(o)
        gid = int(o.getAttribute('gid'))

        FlatBuffGenerated.Decoration.DecorationStart(builder)
        pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
        FlatBuffGenerated.Decoration.DecorationAddPos(builder, pos)
        FlatBuffGenerated.Decoration.DecorationAddRotation(builder, rot)
        FlatBuffGenerated.Decoration.DecorationAddGid(builder, gid)
        objs.decoration.append(FlatBuffGenerated.Decoration.DecorationEnd(builder))

def handle_collision_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y, rotation = get_pos(o)
        ellipse = False
        radius = 0
        polyverts = ""
        poly = 0

        if o.getElementsByTagName('ellipse'):
            ellipse = True
            radius = float(o.getAttribute('width')) / 2.0
            x += math.cos(math.radians(rotation) + math.radians(45)) * radius * math.sqrt(2) # trygonometria 100
            y += math.sin(math.radians(rotation) + math.radians(45)) * radius * math.sqrt(2)
        else:
            polyverts = o.getElementsByTagName('polygon')[0].getAttribute('points')
            polyverts = polyverts.split(' ')
            FlatBuffGenerated.Collision.CollisionStartPolyvertsVector(builder, len(polyverts))
            for v in polyverts:
                vert_x, vert_y = v.split(",")
                vert_x_f = float(vert_x) * GLOBAL_SCALE
                vert_y_f = float(vert_y) * GLOBAL_SCALE
                FlatBuffGenerated.Vec2.CreateVec2(builder, vert_x_f, vert_y_f)                
            poly = builder.EndVector(len(polyverts))

        FlatBuffGenerated.Collision.CollisionStart(builder)
        pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
        FlatBuffGenerated.Collision.CollisionAddPos(builder, pos)
        FlatBuffGenerated.Collision.CollisionAddRotation(builder, rotation)
        FlatBuffGenerated.Collision.CollisionAddEllipse(builder, ellipse)
        FlatBuffGenerated.Collision.CollisionAddRadius(builder, radius * GLOBAL_SCALE)
        FlatBuffGenerated.Collision.CollisionAddPolyverts(builder, poly)
        objs.collision.append(FlatBuffGenerated.Collision.CollisionEnd(builder))


def handle_hidingspots_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y, rotation = get_pos(o)
        ellipse = False
        radius = 0
        polyverts = ""
        poly = 0

        if o.getElementsByTagName('ellipse'):
            ellipse = True
            radius = float(o.getAttribute('width')) / 2.0
            x += math.cos(math.radians(rotation) + math.radians(45)) * radius * math.sqrt(2)
            y += math.sin(math.radians(rotation) + math.radians(45)) * radius * math.sqrt(2)
        else:
            polyverts = o.getElementsByTagName('polygon')[0].getAttribute('points')
            polyverts = polyverts.split(' ')
            FlatBuffGenerated.HidingSpot.HidingSpotStartPolyvertsVector(builder, len(polyverts))
            for v in polyverts:
                vert_x, vert_y = v.split(",")
                vert_x_f = float(vert_x) * GLOBAL_SCALE
                vert_y_f = float(vert_y) * GLOBAL_SCALE
                FlatBuffGenerated.Vec2.CreateVec2(builder, vert_x_f, vert_y_f)
            poly = builder.EndVector(len(polyverts))
        
        name = builder.CreateString(o.getAttribute('name'))

        FlatBuffGenerated.HidingSpot.HidingSpotStart(builder)
        pos = FlatBuffGenerated.Vec2.CreateVec2(builder, x * GLOBAL_SCALE, y * GLOBAL_SCALE)
        FlatBuffGenerated.HidingSpot.HidingSpotAddPos(builder, pos)
        FlatBuffGenerated.HidingSpot.HidingSpotAddEllipse(builder, ellipse)
        FlatBuffGenerated.HidingSpot.HidingSpotAddRadius(builder, radius * GLOBAL_SCALE)
        FlatBuffGenerated.HidingSpot.HidingSpotAddPolyverts(builder, poly)
        FlatBuffGenerated.HidingSpot.HidingSpotAddName(builder, name)
        objs.hidingspots.append(FlatBuffGenerated.HidingSpot.HidingSpotEnd(builder))


def handle_meta_layer(g: minidom.Node, objs: GameObjects, builder: flatbuffers.Builder):
    for o in g.getElementsByTagName('object'):
        x, y, rotation = get_pos(o)

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
    firstgid = int(ts.getAttribute('firstgid'))
    for t in ts.getElementsByTagName('tile'):
        gid = int(t.getAttribute('id')) + firstgid
        name = os.path.basename(t.getElementsByTagName('image')[0].getAttribute('source'))
        fbname = builder.CreateString(name)
        FlatBuffGenerated.Tileinfo.TileinfoStart(builder)
        FlatBuffGenerated.Tileinfo.TileinfoAddName(builder, fbname)
        FlatBuffGenerated.Tileinfo.TileinfoAddGid(builder, gid)
        objs.tileinfo.append(FlatBuffGenerated.Tileinfo.TileinfoEnd(builder))


def process_level(path: str):
    dom = minidom.parse(path)
    map_node = dom.firstChild
    objs = GameObjects([], [], [], [], [], [], [])
    builder = flatbuffers.Builder(1)

    for ts in map_node.getElementsByTagName('tileset'):
        handle_tileset(ts, objs, builder)

    for g in map_node.getElementsByTagName('objectgroup'):
        group_name = g.getAttribute('name')
        if group_name == 'objects':
            handle_objects_layer(g, objs, builder)
        elif group_name == 'decoration':
            handle_decoration_layer(g, objs, builder)
        elif group_name == 'collision':
            handle_collision_layer(g, objs, builder)
        elif group_name == 'hidingspots':
            handle_hidingspots_layer(g, objs, builder)
        elif group_name == 'meta':
            handle_meta_layer(g, objs, builder)
        else:
            print("WARNING: Unknown group {}".format(group_name))
    
    FlatBuffGenerated.Level.LevelStartObjectsVector(builder, len(objs.objects))
    for o in objs.objects:
        builder.PrependUOffsetTRelative(o)
    objectsOff = builder.EndVector(len(objs.objects))

    FlatBuffGenerated.Level.LevelStartDecorationVector(builder, len(objs.decoration))
    for d in objs.decoration:
        builder.PrependUOffsetTRelative(d)
    decorationOff = builder.EndVector(len(objs.decoration))

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

    FlatBuffGenerated.Level.LevelStartTileinfoVector(builder, len(objs.tileinfo))
    for t in objs.tileinfo:
        builder.PrependUOffsetTRelative(t)
    tileinfoOff = builder.EndVector(len(objs.tileinfo))

    FlatBuffGenerated.Level.LevelStart(builder)
    FlatBuffGenerated.Level.LevelAddObjects(builder, objectsOff)
    FlatBuffGenerated.Level.LevelAddDecoration(builder, decorationOff)
    FlatBuffGenerated.Level.LevelAddCollision(builder, collisionOff)
    FlatBuffGenerated.Level.LevelAddHidingspots(builder, hspotsOff)
    FlatBuffGenerated.Level.LevelAddNavpoints(builder, navpointsOff)
    FlatBuffGenerated.Level.LevelAddPlayerwalls(builder, playerwallsOff)
    size = FlatBuffGenerated.Vec2.CreateVec2(builder,
        float(map_node.getAttribute("width")) * float(map_node.getAttribute("tilewidth")) * GLOBAL_SCALE,
        float(map_node.getAttribute("height")) * float(map_node.getAttribute("tileheight")) * GLOBAL_SCALE)
    FlatBuffGenerated.Level.LevelAddSize(builder, size)
    FlatBuffGenerated.Level.LevelAddTileinfo(builder, tileinfoOff)
    level = FlatBuffGenerated.Level.LevelEnd(builder)
    builder.Finish(level)

    buf = builder.Output()

    with open(path[:-3]+"bin", "wb") as f:
        f.write(buf)

# the actual script beginning
for i in os.listdir(levels_dir):
    if i.endswith(".tmx"):
        # level file
        print("Processing level " + i + "...")
        process_level(levels_dir+i)
print("Levelpacker done.")