#!/bin/bash
if [ "$1" != "--keep" ]
then
    rm -rf dfclient-wasm
fi

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm || true
emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-wasm
cd dfclient-wasm
make -j4
