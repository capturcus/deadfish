#!/bin/bash
rm -rf dfclient-native

cmake -DCMAKE_BUILD_TYPE=Debug -S dfclient -B dfclient-native
cd dfclient-native
make -j4

