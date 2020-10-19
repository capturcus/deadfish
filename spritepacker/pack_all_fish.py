#!/usr/bin/python3

import sys, os, subprocess

FISHPATH = sys.argv[1]

imgs = []
folders = []
for f in os.listdir(FISHPATH):
    p = FISHPATH+"/"+f
    if os.path.isdir(p) and "." in f:
        print("including", f)
        folders.append(f)

def mapAnimKeys(key):
    if key == "walk":
        return "a"
    elif key == "run":
        return "b"
    elif key == "attack":
        return "c"
    raise "wrong folder name"

# sort folders so that they are in the order accepted by the client
folders = sorted(folders, key=lambda name: name.split(".")[0]+mapAnimKeys(name.split(".")[1]))
folders = list(map(lambda x: FISHPATH+"/"+x, folders))

subprocess.run(["./spritepacker.py", "--imgsinrow", "10", "--imgsize", "120", "--out", "fish.png", "--paths"] + folders)
