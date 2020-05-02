#!/bin/bash

set -e
# point to your emsdk here
source /home/marcin/Downloads/emsdk/emsdk_env.sh

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S nCine-libraries -B nCine-libraries-wasm
cmake --build nCine-libraries-wasm

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S nCine -B nCine-wasm
cmake --build nCine-wasm

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm || true
emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm
cmake --build dfclient-wasm

