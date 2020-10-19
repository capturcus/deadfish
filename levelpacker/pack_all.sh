#!/bin/bash

LEVELS_DIR="../levels/"

for f in `ls $LEVELS_DIR`
do
    if [ ${f: -4} == ".tmx" ]
    then
        ./levelpacker.py --xml $LEVELS_DIR$f --out $LEVELS_DIR${f%.*}".bin"
    fi
done
