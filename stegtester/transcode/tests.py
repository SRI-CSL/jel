#!/usr/bin/python

import sys

from PIL import Image

from subprocess import call

import os.path

import random, string



culprits = [];

failures = 0

successes = 0


def randomword(length):
   return ''.join(random.choice(string.lowercase) for i in range(length))

def generate_message(name, size):
    file = open(name, "w")
    file.write(randomword(size))
    file.close()
    

def transcode(filein, fileout, target_jpeg_quality):
    img = Image.open(filein)
    img.save(fileout, "JPEG", quality=target_jpeg_quality)
    

def different(filein, fileout):
    sizein  = os.stat(filein).st_size
    sizeout = os.stat(fileout).st_size
    retval = True
    if sizein == sizeout:
        fin = open(filein, 'r')
        sin = fin.read(sizein)
        fout = open(fileout, 'r')
        sout = fout.read(sizein)
        index = 0
        for i , o in zip(sin, sout):
            if not i == o:
                print 'different at index ', index, ' in = ', i, ' out = ', o
                return retval
            index += 1
        retval = False
    os.remove(fileout)
    return retval


def wedge_unwedge(dir, imageno, messagein, messageout):
    global successes, failures
    image = str(imageno)
    message_length  = str(os.stat(messagein).st_size)
    call(['wedge', '-nolength', '-data', messagein, dir + '/' + image + '.jpg',  image + '_jel.jpg'])
    call(['unwedge', '-length', message_length, image + '_jel.jpg', messageout])
    failure = different(messagein, messageout)
    if failure:
        print "wedge-unwedge %s FAILED\n" % format( image + '.jpg')
        exit()
    #else:
    #    print "wedge-unwedge %s OK\n" % format( image + '.jpg')


def wedge_transcode_unwedge(dir, imageno, messagein, messageout):
    global successes, failures, culprits
    image = str(imageno)
    source = dir + '/' + image + '.jpg'
    message_length  = str(os.stat(messagein).st_size)
    call(['wedge', '-nolength', '-data', messagein, dir + '/' + image + '.jpg',  image + '_jel.jpg'])
    transcode(image + '_jel.jpg',  image + '_jel_tr.jpg', 30)
    call(['unwedge', '-length', message_length, image + '_jel_tr.jpg', messageout])
    failure = different(messagein, messageout)
    if failure:
        print "wedge_transcode_unwedge %s FAILED\n" % format( image + '.jpg')
        failures += 1
        culprits.append(source);
    else:
        print "wedge_transcode_unwedge %s OK\n" % format( image + '.jpg')
        successes += 1


generate_message("message_in.bin", 10)

for imageno in range(101, 500):
    image = str(imageno)
    if os.path.exists( 'q30/' + image + '.jpg' ):
        wedge_unwedge('q30', imageno, "message_in.bin", "message_out.bin")
        wedge_transcode_unwedge('q30', imageno, "message_in.bin", "message_out.bin")

print "Successes: ", successes

print "Failures: ", failures

print "Culprits: ", culprits

