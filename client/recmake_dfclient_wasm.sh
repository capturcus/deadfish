#!/bin/bash
rm -rf dfclient-wasm

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm || true
emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm
cd dfclient-wasm
make -j4

