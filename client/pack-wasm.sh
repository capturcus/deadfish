#!/bin/bash

cd dfclient-wasm
rm -rf dist*
mkdir dist
cp deadfishclient* dist
zip -r dist.zip dist

