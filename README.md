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

## development environment

### common requirements

- flatbuffers compiler: `sudo snap install flatbuffers --edge` (this will also install the needed headers for cpp)
- generate sources: `./generate.sh`

### client prerequisites

- install nvm
- `nvm install --lts`, `nvm use --lts`
- `cd client; npm i`
- `npm run server:dev`

### server prerequisites

- `sudo apt install libboost-dev libboost-system-dev libbox2d-dev libwebsocketpp-dev libglm-dev`
- `cd server; ./build.sh`

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

The game can be in one of a few states. All the states are in the client/src/states directory. Boot and preloader were already in the template, they bootstrap the application (I think that preloader is when you see the black screen before the logo loads). The majority of the code is in the gameplay state.

Client/src/utils/fbutil.ts is also an important file. It contains functions for creating and reading flatbuffers. There is no generated way to verify that a binary message is of a certain type, so a function is created with MakeParser for all the generated flatbuffers types that I use in the client.

Client/src/utils/fbutil.ts also contains the game state as an exported variable gameData.

### server

The game can be in one of a few phases: LOBBY, GAME and CLOSING. The CLOSING phase is only there to avoid segfaults while the game is closing. The LOBBY phase is handled in the main.cpp file and the GAME phase is handled in game_thread.cpp.

When the state of the game is changed from LOBBY to GAME, the websocket handler is changed to gameOnMessage (game_thread.cpp).

The entry point is gameThread, and that's where the game loop is. The box2d world is updated, then objects are updated, then objects that marked for deletion are deleted. A mutex must be used because websocket callback (gameOnMessage) will be called on a different thread than the game loop.