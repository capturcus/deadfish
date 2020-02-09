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