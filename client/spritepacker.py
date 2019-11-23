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
SHEETNAME = config['default']['sheetname']
print("imgs in row:", IMGSINROW)
print("img size:", IMGSIZE)
print("img path:", IMGPATH)

imgs = []
for f in os.listdir(IMGPATH):
    if not f.endswith(".png"):
        print("not an image: ", f)
        continue
    ff = open(IMGPATH+"/"+f, 'r+b')
    image = Image.open(ff)
    if image.size[0] != image.size[1]:
        print("image not square:", image.size, f)
    imgs.append(image)
        # cover = resizeimage.resize_cover(image, [200, 100])
        # cover.save('test-image-cover.jpeg', image.format)

rows = int((len(imgs)/IMGSINROW))+1
finalimg = Image.new("RGBA", (IMGSINROW*IMGSIZE, rows*IMGSIZE))
print("finalimg size", finalimg.size)

for i in range(len(imgs)):
    resized = resizeimage.resize_cover(imgs[i], [IMGSIZE, IMGSIZE])
    offset = ((i % IMGSINROW)*IMGSIZE, int((i//IMGSINROW)*IMGSIZE))
    finalimg.paste(resized, offset)

finalimg.save("assets/images/"+SHEETNAME, finalimg.format)