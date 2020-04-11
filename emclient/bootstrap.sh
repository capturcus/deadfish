#!/bin/bash

set -e

emcmake cmake -S nCine-libraries -B nCine-libraries-build
cmake --build nCine-libraries-build

emcmake cmake -S nCine -B nCine-build
cmake --build nCine-build

emcmake cmake -S dfclient -B dfclient-build
cmake --build dfclient-build

