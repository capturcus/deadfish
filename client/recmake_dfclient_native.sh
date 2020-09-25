#!/bin/bash
rm -rf dfclient-native

cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-native
cmake --build dfclient-native

cp ./nCine-native/libncine_d.so dfclient-native
cp ./nCine-external/lib/libGLEWd.so.2.2 dfclient-native/
cp ./nCine-external/lib/libglfw.so.3 dfclient-native/
cp ./nCine-external/lib/libpng16d.so.16 dfclient-native/
cp ./nCine-external/lib/libwebp.so.7 dfclient-native/

ln -s ../../data dfclient-native
