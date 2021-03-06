#!/usr/bin/python3

import os, argparse
from PIL import Image
from resizeimage import resizeimage

parser = argparse.ArgumentParser(description='Packs a folder or a series of folders into a sprite sheet. Supports only PNGs for now.')
parser.add_argument("--imgsinrow", help="how many images to fit in a row", default=10)
parser.add_argument("--imgsize", help="the size of a single sprite in the target spritesheet", default=120)
parser.add_argument("--out", help="path to the resulting spritesheet", required=True)
parser.add_argument("--paths", help="folders to pack into the spritesheet", required=True, nargs="+")

args = parser.parse_args()

imgsize = int(args.imgsize)
imgsinrow = int(args.imgsinrow)

imgs = []

for fol in args.paths:
    print("processing", fol)
    files = []
    for f in os.listdir(fol):
        if f.endswith(".png"):
            files.append(f)
    for f in sorted(files):
        ff = open(fol+"/"+f, 'r+b')
        image = Image.open(ff)
        if image.size[0] != image.size[1]:
            print("image not square:", image.size, f)
        imgs.append(image)

rows = int((len(imgs)/imgsinrow)) + (0 if len(imgs) % imgsinrow == 0 else 1)
finalimg = Image.new("RGBA", (imgsinrow * imgsize, rows * imgsize))
print("finalimg size", finalimg.size)

for i, img in enumerate(imgs):
    resized = resizeimage.resize_cover(img, [imgsize, imgsize])
    offset = ((i % imgsinrow)*imgsize, int((i//imgsinrow)*imgsize))
    finalimg.paste(resized, offset)

finalimg.save(args.out, finalimg.format)
