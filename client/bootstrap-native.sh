#!/bin/bash

set -e

cmake -DCMAKE_BUILD_TYPE=Debug -S nCine-libraries -B nCine-libraries-native
cmake --build nCine-libraries-native

cmake -DCMAKE_BUILD_TYPE=Debug -S nCine -B nCine-native -D CMAKE_PREFIX_PATH=$(pwd)/nCine-external
cmake --build nCine-native

./recmake_dfclient_native.sh
