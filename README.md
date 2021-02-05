# deadfish

## Game mechanics

You are a sea creature on the bottom of the ocean. There are multiple species of creatures, but only one creature of each species is a player, the rest are NPCs (or civilians). The objective of the game is to identify which creature is another player and kill them, for which you get points. For killing civilians the points are deducted.

Game interface:
- **left click** to move
- **right click** to kill (only within a certain radius from the target creature)
- **q** to sprint (unless you're Moon)
- **a** to show highscores
- creatures that are within attacking range are marked by a blue circle
- creatures that you're hovering over are marked by a gray circle
- a creature that is currently targeted by you is marked by a red circle

## How to build the game

The build is dockerized. 

 - install docker on your system
 - download the `df-dev-env` image from [Google Drive](https://drive.google.com/file/d/1hKNhHi3mjIonS6vOdkzUOU_47f9bLSNX/view?usp=sharing)
 - load the docker image: `docker load < df-dev-env.tar.gz`
 - clone the repository
 - go to `deadfish/env`, run `./run-docker.sh`, this will:
     - create a docker container to execute the rest of the steps in it
     - download submodules
     - build the server
     - build the native client
     - download and install emscripten sdk
     - build the wasm client
 - after the build is completed you can run the game from the host system (not inside the container):
     - use the `run_server.sh` script from the `deadfish/server/build` directory (not `deadfish/server`).
     - you can launch the client directly from the `deadfish/client/dfclient-native` directory
     - you can run a http server (for instance `python -m SimpleHTTPServer`) in the `deadfish/client/dfclient-wasm` directory to load and run the wasm client
 - to build things after introducing some changes run `make` in the main deadfish directory, there are 3 targets:
     - server
     - client-native
     - client-wasm
 - you can also run `make` manually inside the container in the respective directories:
     - server: `/deadfish/server/build`
     - native client: `/deadfish/client/dfclient-native`
     - wasm client: `/deadfish/client/dfclient-wasm`
 - it is recommended to install the `dfctl` control script: `cd env; sudo ./dfctl install`, after installing it run `dfctl` to see the usage
     - `dfctl` supports word completion, but you have to reboot bash after installation (eg. with `exec bash`)

## Project files description

### Communication between server and client

- client sends a JoinRequest
- server replies with InitMetadata
- the server sends an additional InitMetadata for all players that join or leave
- the client sends a PlayerReady
- when all the players are ready the server sends the Level message, clients go to the gameplay state
- every tick (20fps) the server sends a WorldState message
- on every action that the user executes the client sends a Command* message, i.e. CommandMove, CommandRun or CommandKill
- when a kill attempted (a CommandKill) is sent by the client, the server either will send a TooFarToKill or not
- when the player has the intent of killing and moves close enough so that the kill is executed:
    - a KilledNPC is sent or a DeathReport is sent depending on whether a Civilian or a Player was killed
    - a HighscoreUpdate is sent

### Client

to be documented in the very near future

### Server

The game can be in one of a few phases: LOBBY, GAME and CLOSING. The CLOSING phase is only there to avoid segfaults while the game is closing. The LOBBY phase is handled in the main.cpp file and the GAME phase is handled in game_thread.cpp.

When the state of the game is changed from LOBBY to GAME, the websocket handler is changed to gameOnMessage (game_thread.cpp).

The entry point is gameThread, and that's where the game loop is. The box2d world is updated, then objects are updated, then objects that marked for deletion are deleted. A mutex must be used because websocket callback (gameOnMessage) will be called on a different thread than the game loop.

## Level creation

### Overview

Levels are created in [Tiled](https://www.mapeditor.org/). However, contrary to the editor name, the maps are generally not tile based and all interactive objects are placed freely. The maps contain visible objects: _obstacles_ (e.g. stones), _decorations_ (e.g. sand patterns) and _hiding spot_ visual representation (e.g. bushes), as well as invisible, related to the game mechanics: _collision_ objects, _hiding spot_ mechanical objects, _playerwalls_ and _waypoints_.

It is __very__ important to use Tiled __version 1.4.0 or higher__ (otherwise there will be positioning problems with every rotated object.) The newest versions are available on `snap` or on Tiled [website](https://www.mapeditor.org/) (the newest version available via `apt` is currently below 1.4.0).

### Pipeline

The levels are created in Tiled and saved in the __.tlx__ format (Tiled XML). This is then parsed by [`levelpacker.py`](./levelpacker/levelpacker.py) and packed neatly into a flatbuffer, defined in [`deadfish.fbs`](./deadfish.fbs), producing a __.bin__ file. The flatbuffer bin is then imported and parsed by server upon launch. When clients connect, server packs and sends a simplified version for them, cutting out all potentially sensitive server-side information.

### Structure

Because of the aforementioned processing flow, the level's internal structure is very strict. Everything must be named and set up properly, otherwise the server won't be able to load the level or client won't be able to properly display it.

Tiled uses **tilesets** to group object "classes" and **layers** to group object instances in the map. `levelpacker.py` recognizes the layers by name.
The levels contain ~~two~~ ~~four~~ ~~five~~ six basic layers:
 - `tiles`
     - The only tile layer. Created for background purposes, rendered under everything else.
 - `decoration`
     - The second visible layer, containing (surprise, surprise) decorations. A bit similar to `tiles` in purpose, but objects (tiles) here can be placed freely. Everything on this layer will be rendered **under** the player.
 - `objects`
     - The third visible layer, this layer contains all main visible stuff, like bushes, stones etc. Everything from this layer will be rendered **over** the player.
 - `collisionMasks`
     - This layer contains objects representing collision bodies. They are invisible.
 - `hidingSpots`
     - As the name suggests, this layer defines hiding spots, but from a mechanical point of view - objects that obstruct sight by default, but not when you are inside. They are not visible either.
 - `meta`
     - This is the layer for the other general mechanics. Here go the waypoints and playerwalls.

#### General remarks

Objects on visible layers are **sprites**. Sprites can be added to Tiled via _tilesets_. Tilesets in Tiled can be **external** (function as a separate file) or **embedded** in map. Because we want the map to have the full info (so the server can manage it all and client doesn't have to access any file), all tilesets should be embedded. External tilesets can be easily imported to a map (via "Map->Add External Tileset...") and then embedded, so you can use any and all existing ones or make a new, custom tileset to use across your levels.

Invisible objects (those from `collisionMasks`, `hidingSpots` and `meta` layers) are **shapes**. Collision objects and hidingspots can be a circle (not an ellipse!), a rectangle or a polygon, waypoints should be circles and playerwalls should be rectangles.

A circle is understood as a Tiled ellipse with equal width and height. If an object supposed to be a circle turns out to be an ellipse, `levelpacker.py` will crash with a message regarding the object.

Polygons created in Tiled are later processed by box2d, so objects created from these shapes will stick to the box2d rules, which are:
 - _only convex shapes_ -- any "hole" in a polygon will be filled by box2d so as to form a convex shape anyway,
 - _no more than 8 vertices_ -- this is defined by b2dSettings.h somewhere deep in the box2d library. Checked by `levelpacker.py`.

#### Tiles

For the tile layer to work properly, some settings need to be correctly set in the Map Properties. Namely:
 - "Tile Width" and "Tile Height" (kinda obvious, but still)
 - "Infinite" set to *__false__*
 - "Tile Layer Format" set to **CSV**

For some reason the map dimensions in tiles are more stubborn when it comes to change, but that can be done with "Map->Resize Map..." menu option.

#### Colliding objects

"Physical" objects, that are supposed to collide with players and mobs, need to have their visual and collision parts constructed separately - place a sprite (or a few) on the `objects` layer and then create a collision mask on `collisionMasks` layer with a circle, box or polygon shape. This may seem like unnecessary work, but it gives much more freedom and it's not that tedious if you consider [Tips & Tricks](#tips-&-tricks).

Using this layer you can also create "overlaying decorations" - just add an object without a collision mask.

#### Hiding spots

Hiding spots need to be created in two parts, visible and "mechanical", similarly to collisions. Sprites of a group that makes a hiding spot are linked to the hidingspot object by its name. Create a shape on `hidingSpots` layer and give it a `name` property. Then and add a custom property `hspotName` to all sprites that make the hiding spot with the value equal to the aforementioned shape's name. [Tips & Tricks](#tips-&-tricks) might help with easy creation.

Hiding spot sprites are rendered over decorations and regular objects, so you can create hidden obstacles in large hiding spots, for example.

#### "Meta" layer 

The objects here are simple shapes, just like `collisionMasks` and `hidingSpots`. Waypoints are circles, playerwalls are rectangles. They are recognized by their `type` object property, that being `waypoint` and `playerwall` respectively.

Waypoints are mob destinations - every mob is always headed towards one of those. They are organized in a form of directed graph: every waypoint has a __name__ object property and a custom property `wps`, which specifies its graph neighbours through a comma-separated list of waypoint names.

Additionally, waypoints can be spawn locations for mobs and/or players. Every waypoint that has a `isplayerspawn` custom property defined and set to true becomes a potential player spawn location, same goes for mobs and waypoints with `isspawn`. Spawns are waypoints, they don't exist independently. They can have no graph connections if that's desired, but still need to have the `type` property set to `waypoint`.

Playerwalls are invisible walls for players that mobs can freely walk through. They serve as a way to keep players inside the level area and give the mobs the ability to spawn outside of the players' sight.

### Caveats

One important detail is Object Alignment. It's a feature from Tiled v1.4.0 up. It's a tileset property that defines where is the "beginning" of the tiles. By default it's set to "Unspecified" for backwards compatibility, which sets the (0, 0) point of every tile to bottom left. For rotation to be interpreted properly by `levelpacker.py` and for compatibility with box2d, this needs to be set to "Top Left".

### Tips & Tricks (important)

 - A good place to start creating a new level is [`template.tmx`](levels/template.tmx). It's an empty level with all the layers properly set.

 - Most `levelpacker.py` errors come from objects accidentally added to a wrong layer. You can move them to the correct layer by selecting them and clicking the layer button in the object panel.

 - If you have trouble selecting the needed objects, because something blocks your view (or simply because of a clusterfuck), you can try hiding overlaying layers, such as `collisionMasks`, `meta` or `hidingSpots`, or even single objects. Another good way of selecting things is through the objects panel on the right - it also shows the selection rectangle on the map view. If you need to "click through" and select something underneath an overlaying object, use Alt + Click.

 - Bulk object edition is supported - you can select a group of objects (e.g. a bush cluster) and set their properties simultaneously (e.g. their `hspotName`).

 - Tiled supports easy duplication mechanics. You can make an object (e.g. a properly set up hiding spot sprite) or a group (e.g. an obstacle with its collision mask) and then copy the whole thing with Ctrl+C. Pressing Ctrl+V pastes the object (group) on the cursor location, which allows for a quite convenient creation workflow by setting up a template, copying and then spamming Ctrl+V, pasting it all over the place. Copied objects are selected on creation, so you can easily adjust the position if necessary.

 - All objects fully support rotation. If you need to, you can select a whole part of a level and just rotate it, or copy it somewhere with added rotation.

 - While swithing focus between object layers is as easy as simply clicking an object, the tile layer editing tools are drastically different. To switch between tile and object editing modes, choose the right layer from the menu on the right.

 - When in tile edition mode, you can make a temporary custom brush from alredy painted parts. Simply right-click and drag on the map to select the brush template. Useful for covering large areas with similar stuff.

 - If you discover something worth mentioning, expand the list ;)

## Matchmaking

When Deadfish will be deployed on a Kubernetes cluster using Agones there will be a need for a matchmaker to coordinate players into games. The matchmaker is stored in a [separate repository](https://github.com/capturcus/deadfish-matchmaker).

The client binary is shipped with an .ini file that contains the default address of the matchmaker.

### Matchmaking process:

1. The user inputs a desired username an clicks `find game`.
2. A HTTP GET request is sent to the matchmaker on endpoint `/matchmake`
3. The matchmaker uses internal Kubernetes API to find a suitable game server to add a new player to. The server must fit the following criteria:
     - The game has not started yet (the server is in the lobby state).
     - The number of players has not exceeded the desired number of players in the lobby (for now hardcoded to 6).
4. If the matchmaker is unable to find a server that fits these criteria, a new Agones GameServer is allocated using the Kubernetes API.
5. The gameserver's IP and port is sent to the game client as a response to the `/matchmake` GET request in a JSON:
```
{
    "status": "ok",
    "address": "<the gameserver's ip address>",
    "port": "<the gameserver's port>"
}
```
If an error occurred in the matchmaking process the `status` field is set to `error` and there is an `error` field with additional description that will be shown to the user.
6. The game client connects to the supplied address and port.
