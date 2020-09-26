#!/bin/bash
docker run -d --name deadfish-env \
	-v $(realpath ..):/deadfish \
	df-dev-env

docker exec --user ubuntu -it deadfish-env bash -c 'cd /deadfish && git submodule update --init && ./generate.sh && cd server && ./build.sh && cd /deadfish/client && ./bootstrap-native.sh && cd / && git clone https://github.com/emscripten-core/emsdk && cd emsdk && ./emsdk install latest && ./emsdk activate latest && source /emsdk/emsdk_env.sh && cd /deadfish/client && ./bootstrap-wasm.sh'
