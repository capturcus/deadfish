# deadfish

## common requirements

- flatbuffers compiler: `sudo snap install flatbuffers --edge` (this will also install the needed headers for cpp)
- generate sources: `./generate.sh`

## client prerequisites

- install nvm
- `nvm install --lts`, `nvm use --lts`
- `cd client; npm i`
- `npm run server:dev`

## server prerequisites

- `sudo apt install libboost-dev libboost-system-dev libbox2d-dev libwebsocketpp-dev libglm-dev`
- `cd server; ./build.sh`