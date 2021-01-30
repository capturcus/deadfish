#!/bin/bash

set -e

docker exec --user ubuntu -it deadfish-env bash -c 'cd /deadfish/levelpacker && ./levelpacker.py && cd /deadfish/server && mkdir -p build-prod && cd build-prod && make -j && cp /usr/lib/x86_64-linux-gnu/libBox2D.so.2.3.0 . && cp /usr/local/lib/libboost_system.so.1.74.0 . && cp /usr/local/lib/libboost_program_options.so.1.74.0 .'

cd ../server/build-prod
cp deadfishserver ../../env/deadfish-server
cp *.so.* ../../env/deadfish-server
cd ..
cd ../env/deadfish-server
rm -rf levels
mkdir levels
cp ../../levels/*.bin levels

docker rmi deadfish-server || true
docker build --tag deadfish-server .
