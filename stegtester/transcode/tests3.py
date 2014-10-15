#!/usr/bin/python

import sys
from PIL import Image

from subprocess import call

import os.path

randmsg = '../../bin/randmsg'

msglen = 18000

flist = '10,9,8,3'
flist = '18,17,16,10'
flist = '18,16,10,3'

seed = 20

ecclen = 10

culprits = [];

failures = 0

successes = 0

#fmap = N.zeros(256)
#emap = N.zeros(256)

def byte_hamming(char1, char2):
    i = 0
    k1 = ord(char1)
    k2 = ord(char2)

    x = k1 ^ k2

    for k in range(0,8):
        if ( x & 1 ): i = i + 1
        x = x >> 1
    
#    fmap[k1] += 1
#    if (i > 0): emap[k1] = emap[k1] + 1

    return i


#
# Return the Hamming distance between two strings:
#
def hamming(str1, str2):
    if (len(str1) != len(str2)):
        print "Different string lengths?"
        print "Length of string 1 = {:d}".format(len(str1))
        print "Length of string 2 = {:d}".format(len(str2))

    sum = 0
    for k in range(0,min(len(str1), len(str2))):
        sum += byte_hamming(str1[k], str2[k])

    return sum, len(str1)*8


def diff(file1, file2):
    f1 = open(file1, "rb")
    f2 = open(file2, "rb")

    nbits, total = hamming(f1.read(), f2.read())

    f1.close()
    f2.close()
    return (nbits > 0), nbits, total



def make_randmsg(nbytes):
    global randmsg
    f = open('/tmp/wedgetest.msg', "wb")
    call([randmsg, str(nbytes)], stdout=f)
    f.close()


def transcode_1(filein, fileout, target_jpeg_quality):
    img = Image.open(filein)
    img.save(fileout, "JPEG", quality=target_jpeg_quality)

def transcode_2(filein, fileout, target_jpeg_quality):
    call(['convert', filein, '-quality', str(target_jpeg_quality), fileout])
    

def wedge_unwedge(dir, imageno):
    global successes
    global failures
    image = str(imageno)
    make_randmsg(msglen)
    call(['wedge', '-data', '/tmp/wedgetest.msg', '-ecc', str(ecclen), '-nolength',  '-freq', flist, dir + '/' + image + '.jpg',  image + '_jel.jpg'])
    call(['unwedge', '-length', str(msglen), '-ecc', str(ecclen), '-freq', flist, image + '_jel.jpg', 'text.txt'])
#    failure = call(['diff', 'text.txt', '/tmp/wedgetest.msg'])
    [failure, nbits, tot] = diff('text.txt', '/tmp/wedgetest.msg')
    if failure:
        print "wedge-unwedge {0} FAILED ({1} bits)\n".format( image + '.jpg', nbits)
        failure += 1
    else:
        print "wedge-unwedge %s OK\n" % format( image + '.jpg')
        successes += 1

def wedge_transcode_unwedge(dir, imageno):
    global successes
    global failures
    global culprits
    image = str(imageno)
    source = dir + '/' + image + '.jpg'
    make_randmsg(msglen)
    call(['wedge', '-data', '/tmp/wedgetest.msg', '-ecc', str(ecclen), '-nolength', '-freq', flist, source,  image + '_jel.jpg'])
    transcode_1(image + '_jel.jpg',  image + '_jel_tr.jpg', 30)
    call(['unwedge', '-length', str(msglen), '-ecc', str(ecclen), '-freq', flist, image + '_jel_tr.jpg', 'text.txt'])
#    failure = call(['diff', 'text.txt', '/tmp/wedgetest.msg'])
    [failure, nbits, tot] = diff('text.txt', '/tmp/wedgetest.msg')
    if failure:
        print "wedge_transcode_unwedge {0} FAILED ({1} bits)\n".format( image + '.jpg', nbits)
        failures += 1
        culprits.append(source);
        call(['convert', '-compose', 'subtract', image + '_jel.jpg', image + '_jel_tr.jpg', image + '_diff.jpg'])

    else:
        print "wedge_transcode_unwedge %s OK\n" % format( image + '.jpg')
        successes += 1

for imageno in range(101, 500):
    image = str(imageno)
    if os.path.exists( 'q30-squashed/' + image + '.jpg' ):
        wedge_unwedge('q30-squashed', imageno)
        wedge_transcode_unwedge('q30-squashed', imageno)

print "Successes: ", successes

print "Failures: ", failures


print "Culprits: ", culprits

