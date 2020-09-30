# deadfish

## game mechanics

You are a sea creature on the bottom of the ocean. There are multiple species of creatures, but only one creature of each species is a player, the rest are NPCs (or civilians). The objective of the game is to identify which creature is another player and kill them, for which you get points. For killing civilians the points are deducted.

Game interface:
- **left click** to move
- **right click** to kill (only within a certain radius from the target creature)
- **q** to sprint (unless you're Moon)
- **a** to show highscores
- creatures that are within attacking range are marked by a blue circle
- creatures that you're hovering over are marked by a gray circle
- a creature that is currently targeted by you is marked by a red circle

## how to build the game

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

## project files description

### communication between server and client

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

### client

to be documented in the very near future

### server

The game can be in one of a few phases: LOBBY, GAME and CLOSING. The CLOSING phase is only there to avoid segfaults while the game is closing. The LOBBY phase is handled in the main.cpp file and the GAME phase is handled in game_thread.cpp.

When the state of the game is changed from LOBBY to GAME, the websocket handler is changed to gameOnMessage (game_thread.cpp).

The entry point is gameThread, and that's where the game loop is. The box2d world is updated, then objects are updated, then objects that marked for deletion are deleted. A mutex must be used because websocket callback (gameOnMessage) will be called on a different thread than the game loop.