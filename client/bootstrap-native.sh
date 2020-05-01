#!/bin/bash

set -e

cmake -DCMAKE_BUILD_TYPE=Debug -S nCine-libraries -B nCine-libraries-native
cmake --build nCine-libraries-native

cmake -DCMAKE_BUILD_TYPE=Debug -DNCINE_WITH_THREADS=ON -S nCine -B nCine-native -D CMAKE_PREFIX_PATH=$(pwd)/nCine-external
cmake --build nCine-native

cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-native
cmake --build dfclient-native

