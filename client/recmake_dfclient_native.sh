#!/bin/bash
if [ "$1" != "--keep" ]
then
    rm -rf dfclient-native
fi

cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-native
cmake --build dfclient-native

cp ./nCine-native/libncine_d.so dfclient-native
cp ./nCine-external/lib/libGLEWd.so.2.2 dfclient-native/
cp ./nCine-external/lib/libglfw.so.3 dfclient-native/
cp ./nCine-external/lib/libpng16d.so.16 dfclient-native/
cp ./nCine-external/lib/libwebp.so.7 dfclient-native/
cp ./nCine-external/lib/liblua5.3.so dfclient-native/
cp /usr/local/lib/libboost_system.so.1.74.0 dfclient-native/

if [ "$1" != "--keep" ]
then
    ln -s ../../data dfclient-native
fi
