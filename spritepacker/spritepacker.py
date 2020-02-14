#!/usr/bin/python3

import os
from PIL import Image
from resizeimage import resizeimage

import configparser
config = configparser.ConfigParser()
config.read('spritepacker.ini')

IMGSINROW = int(config['default']['imgsinrow'])
IMGSIZE = int(config['default']['imgsize'])
IMGPATH = config['default']['imgpath']
DESTINATION = config['default']['destination']
ASSETS = "../assets/"
print("imgs in row:", IMGSINROW)
print("img size:", IMGSIZE)
print("img path:", IMGPATH)

imgs = []
folders = []
for f in os.listdir(IMGPATH):
    # print(f)
    if os.path.isdir(ASSETS+f):
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

print(folders)

for fol in folders:
    files = []
    for f in os.listdir(ASSETS+fol):
        if f.endswith(".png"):
            files.append(f)
    for f in sorted(files):
        print(f)
        ff = open(IMGPATH+"/"+fol+"/"+f, 'r+b')
        image = Image.open(ff)
        if image.size[0] != image.size[1]:
            print("image not square:", image.size, f)
        imgs.append(image)

rows = int((len(imgs)/IMGSINROW))+1
finalimg = Image.new("RGBA", (IMGSINROW*IMGSIZE, rows*IMGSIZE))
print("finalimg size", finalimg.size)

for i in range(len(imgs)):
    resized = resizeimage.resize_cover(imgs[i], [IMGSIZE, IMGSIZE])
    offset = ((i % IMGSINROW)*IMGSIZE, int((i//IMGSINROW)*IMGSIZE))
    finalimg.paste(resized, offset)

finalimg.save(DESTINATION+".[{},{}]".format(IMGSIZE, IMGSIZE)+".png", finalimg.format)
