#!/bin/bash
rm -rf dfclient-native
rm -rf dfclient-wasm

cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-native
cd dfclient-native
make -j4
cd ..
emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm
cd dfclient-wasm
make -j4