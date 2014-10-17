#!/usr/bin/python

#
# Simple regression tester for wedge.
#
import os
import sys
import time
from subprocess import *
from random import *
import numpy as N
import itertools

# Use hamming distance to check messages

fmap = N.zeros(256)
emap = N.zeros(256)

def byte_hamming(char1, char2):
    i = 0
    k1 = ord(char1)
    k2 = ord(char2)

    x = k1 ^ k2

    for k in range(0,8):
        if ( x & 1 ): i = i + 1
        x = x >> 1
    
 #   fmap[k1] += 1
 #   if (i > 0): emap[k1] = emap[k1] + 1

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


def hamming_files(file1, file2):
    f1 = open(file1, "rb")
    f2 = open(file2, "rb")

    nbits, total = hamming(f1.read(), f2.read())

    f1.close()
    f2.close()
    return nbits, total



if ( not os.path.isdir('tmp') ):
    os.mkdir('tmp')

if ( not os.path.isdir('tmp/raw') ):
    os.mkdir('tmp/raw')
    
if ( not os.path.isdir('tmp/msg') ):
    os.mkdir('tmp/msg')
    

#
# Tester will start with a base quality of 30 and iterate over
# qualities to verify preservation of the message using hamming
# distance, strictly using wedge/unwedge.
#

baseq = 30

def file_length(file):
    f = open(file, "rb")
    x = len(f.read())
    f.close()
    return x


def filename_for_quality(base, quality=baseq):
    return base+"-"+str(quality)+".jpg"

#
# Convert the test file "tree640.pnm" to jpeg at a specified quality.
#
def pnm_to_jpeg(quality, infile="data/images/tree640.pnm", outfile=filename_for_quality("tree640")):
    err = open("/dev/null", "w")
    call(["convert", infile, "-quality", str(quality), outfile], stderr=err)
    err.close()


def wcap(filename):
    print "Running 'wcap "+filename+"'"
    return int(check_output(["wcap", filename]))


msgfile = None
jpeg_file = filename_for_quality("tmp/tree640")
pnm_to_jpeg(baseq, outfile=jpeg_file)
msglen = wcap(jpeg_file)

print "Capacity of", jpeg_file, "is", msglen, "bytes."
print "Generating random message file in 'msg.dat'"
msglen = msglen - 16
msgfile = "tmp/msg.dat"

f = open(msgfile, "wb")
call(["randmsg", str(msglen)], stdout=f)
f.close()

################################################################

# Selection of frequency components.  This needs to be more
# principled!

# For now, depend on the base quality to determine the best
# frequencies:

# flist_set = [None, '18,17,16,10', '10,9,8,3']
flist_set = [None]

################################################################

def wedge(before, after, msgfile, flist=None, quality=None, seed=None, embed_length=False):
    cmd_arglist = ["-data", msgfile, before, after]

    if (not embed_length):
        cmd_arglist = ["-nolength"] + cmd_arglist

    if (flist != None):
        cmd_arglist = ["-freq", flist] + cmd_arglist

    if (quality != None):
        cmd_arglist = ["-quality", str(quality)] + cmd_arglist

    if (seed != None):
        cmd_arglist = ["-seed", str(seed)] + cmd_arglist

    call(["wedge"] + cmd_arglist)



def unwedge(imagefile, msgfile, nbytes, flist=None, seed=None, embed_length=False):
    cmd_arglist = [imagefile, msgfile]

    if (not embed_length):
        cmd_arglist = ["-length", str(nbytes)] + cmd_arglist

    if (flist != None):
        cmd_arglist = ["-freq", flist] + cmd_arglist

    if (seed != None):
        cmd_arglist = ["-seed", str(seed)] + cmd_arglist

    call(["unwedge"] + cmd_arglist)




def transcode_q(basefile, tofile, toq, backfile, backq):
    call(["convert", basefile, "-quality", str(toq), tofile])
    call(["convert", tofile, "-quality", str(backq), backfile])



cover = filename_for_quality("tmp/raw/tree640", baseq)
pnm_to_jpeg(baseq, "data/images/tree640.pnm", cover)
base_mfile = filename_for_quality("tmp/msg/tree640", baseq)

print "# quality, hamming distance"
print "# Using message length of ", msglen, "bytes."

toterr = 0

for flist in flist_set:
    print
    if (flist==None):
        print "# Frequencies: None (auto-selected)"
    else:
        print "# Frequencies: ", flist

    qhi = 75
    cover_high = filename_for_quality("tmp/raw/tree640", qhi)
    pnm_to_jpeg(qhi, "data/images/tree640.pnm", cover_high)

    print "Fixed frequencies, no length embedding:"
    for q in range(baseq,100):
        cover = filename_for_quality("tmp/raw/tree640", q)
        pnm_to_jpeg(q, "data/images/tree640.pnm", cover)
        base_mfile = filename_for_quality("tmp/msg/tree640", q)
        wedge(cover, base_mfile, msgfile, flist)

        msgout = "tmp/msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

        base_mfile = filename_for_quality("tmp/msg/tree640a", q)
        wedge(cover_high, base_mfile, msgfile, flist, q)

        msgout = "tmp/msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

    print " "

    print "Fixed frequencies, with length embedded:"
    for q in range(baseq,100):
        cover = filename_for_quality("tmp/raw/tree640", q)
        pnm_to_jpeg(q, "data/images/tree640.pnm", cover)
        base_mfile = filename_for_quality("tmp/msg/tree640", q)
        wedge(cover, base_mfile, msgfile, flist, embed_length=True)

        msgout = "tmp/msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen, embed_length=True)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

        base_mfile = filename_for_quality("tmp/msg/tree640a", q)
        wedge(cover_high, base_mfile, msgfile, flist, q, embed_length=True)

        msgout = "tmp/msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen, embed_length=True)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

    print " "
    seed = int(time.time())


    print "With PRN seed: ", seed
    for q in range(baseq,100):
        cover = filename_for_quality("tmp/raw/tree640", q)
        pnm_to_jpeg(q, "data/images/tree640.pnm", cover)
        base_mfile = filename_for_quality("tmp/msg/tree640", q)
        wedge(cover, base_mfile, msgfile, flist, seed=seed)

        msgout = "tmp/msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen, seed=seed)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

        base_mfile = filename_for_quality("tmp/msg/tree640a", q)
        wedge(cover_high, base_mfile, msgfile, flist, q, seed=seed)

        msgout = "tmp/msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen, seed=seed)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

print
if (toterr > 0):
    print "Test failed.  Number of bits in error,", toterr, "is nonzero."
else:
    print "Success.  No errors."
