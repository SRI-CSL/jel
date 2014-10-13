#!/usr/bin/python

import sys
from PIL import Image

print 'Number of arguments:', len(sys.argv), 'arguments.'
print 'Argument List:', str(sys.argv)

if len(sys.argv) == 4:
     filein = sys.argv[1]
     fileout = sys.argv[2]
     target_jpeg_quality = int(sys.argv[3])
     img = Image.open(filein)
     print "img info: ", img.format, img.size, img.mode
     # img.save(fileout, "JPEG", quality=target_jpeg_quality)
     img.save(fileout, "JPEG")
else:
     print "Usage: jpeg.py <input file> <output file> <target jpeg quality>"

