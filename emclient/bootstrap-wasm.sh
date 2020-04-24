#!/bin/bash

set -e

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S nCine-libraries -B nCine-libraries-wasm
cmake --build nCine-libraries-wasm

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S nCine -B nCine-wasm
cmake --build nCine-wasm

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm
cmake --build dfclient-wasm

