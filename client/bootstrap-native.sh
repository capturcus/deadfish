#!/bin/bash

set -e

cmake -DCMAKE_BUILD_TYPE=Debug -S nCine-libraries -B nCine-libraries-native
cmake --build nCine-libraries-native

cmake -DCMAKE_BUILD_TYPE=Debug -S nCine -B nCine-native -D CMAKE_PREFIX_PATH=$(pwd)/nCine-external
cmake --build nCine-native

cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-native
cmake --build dfclient-native

cp ./nCine-native/libncine_d.so dfclient-native
cp ./nCine-external/lib/libGLEWd.so.2.2 dfclient-native/
cp ./nCine-external/lib/libglfw.so.3 dfclient-native/
cp ./nCine-external/lib/libpng16d.so.16 dfclient-native/
cp ./nCine-external/lib/libwebp.so.7 dfclient-native/
