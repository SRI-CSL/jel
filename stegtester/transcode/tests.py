#!/usr/bin/python

import sys
from PIL import Image

from subprocess import call

import os.path

failures = 0

successes = 0

def transcode(filein, fileout, target_jpeg_quality):
    img = Image.open(filein)
    img.save(fileout, "JPEG", quality=target_jpeg_quality)
    

def wedge_unwedge(dir, imageno):
    global successes
    global failures
    image = str(imageno)
    call(['wedge', '-message', '0123456789', dir + '/' + image + '.jpg',  image + '_jel.jpg'])
    call(['unwedge', image + '_jel.jpg', 'text.txt'])
    failure = call(['diff', 'text.txt', 'orig.bin'])
    if failure:
        print "wedge-unwedge %s FAILED\n" % format( image + '.jpg')
        failure += 1
    else:
        print "wedge-unwedge %s OK\n" % format( image + '.jpg')
        successes += 1

def wedge_transcode_unwedge(dir, imageno):
    global successes
    global failures
    image = str(imageno)
    call(['wedge', '-message', '0123456789', dir + '/' + image + '.jpg',  image + '_jel.jpg'])
    transcode(image + '_jel.jpg',  image + '_jel_tr.jpg', 30)
    call(['unwedge', image + '_jel_tr.jpg', 'text.txt'])
    failure = call(['diff', 'text.txt', 'orig.bin'])
    if failure:
        print "wedge_transcode_unwedge %s FAILED\n" % format( image + '.jpg')
        failures += 1
    else:
        print "wedge_transcode_unwedge %s OK\n" % format( image + '.jpg')
        successes += 1

for imageno in range(101, 500):
    image = str(imageno)
    if os.path.exists( 'q30/' + image + '.jpg' ):
        wedge_unwedge('q30', imageno)
        wedge_transcode_unwedge('q30', imageno)

print "Failures: ", failures

print "Successes: ", successes
