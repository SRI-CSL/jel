#!/usr/bin/python

#
# Simple regression tester for wedge.
#
import os
import sys
from subprocess import *
from random import *
from hamming import *

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
def pnm_to_jpeg(quality, infile="tree640.pnm", outfile=filename_for_quality("tree640")):
    err = open("/dev/null", "w")
    call(["convert", infile, "-quality", str(quality), outfile], stderr=err)
    err.close()


def wcap(filename):
    print "Running 'wcap "+filename+"'"
    return int(check_output(["wcap", filename]))


input_image = "tree640.pnm"
base_image = input_image
msgfile = None
jpeg_file = filename_for_quality("tree640")
pnm_to_jpeg(baseq, outfile=jpeg_file)
msglen = wcap(jpeg_file)

print "Capacity of", jpeg_file, "is", msglen, "bytes."
print "Generating random message file in 'msg.dat'"
msglen = msglen - 16
msgfile = "msg.dat"

f = open(msgfile, "wb")
call(["../bin/randmsg", str(msglen)], stdout=f)
f.close()

################################################################

# Selection of frequency components.  This needs to be more
# principled!

# For now, depend on the base quality to determine the best
# frequencies:

# flist_set = [None, '18,17,16,10', '10,9,8,3']
flist_set = [None]

################################################################

if ( not os.path.isdir('raw') ):
    os.mkdir('raw')
    
if ( not os.path.isdir('msg') ):
    os.mkdir('msg')
    

def wedge(before, after, msgfile, flist=None, quality=None):
    pre = ["wedge", "-nolength", "-data", msgfile]
    post = [before, after]
    mid = []

    if (flist != None): mid += ["-freq", flist]

    if (quality != None): mid += ["-quality", str(quality)]

    call(pre + mid + post)



def unwedge(imagefile, msgfile, nbytes, flist=None):
    if (flist == None):
        call(["unwedge",
              "-length", str(nbytes),
              imagefile,
              msgfile])
    else:
        call(["unwedge",
              "-freq", flist,
              "-length", str(nbytes),
              imagefile,
              msgfile])

def transcode_q(basefile, tofile, toq, backfile, backq):
    call(["convert", basefile, "-quality", str(toq), tofile])
    call(["convert", tofile, "-quality", str(backq), backfile])


cover = filename_for_quality("raw/tree640", baseq)
pnm_to_jpeg(baseq, "tree640.pnm", cover)
base_mfile = filename_for_quality("msg/tree640", baseq)

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
    cover_high = filename_for_quality("raw/tree640", qhi)
    pnm_to_jpeg(qhi, "tree640.pnm", cover_high)

    for q in range(baseq,100):
        cover = filename_for_quality("raw/tree640", q)
        pnm_to_jpeg(q, "tree640.pnm", cover)
        base_mfile = filename_for_quality("msg/tree640", q)
        wedge(cover, base_mfile, msgfile, flist)

        msgout = "msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

        base_mfile = filename_for_quality("msg/tree640a", q)
        wedge(cover_high, base_mfile, msgfile, flist, q)

        msgout = "msg/msgout-"+str(q)+".dat"
        unwedge(base_mfile, msgout, msglen)
        nb, total = hamming_files(msgout, msgfile)
        print "{0}:{1} ".format(q,nb),
        sys.stdout.flush()
        toterr += nb

print
if (toterr > 0):
    print "Test failed.  Number of bits in error,", toterr, "is nonzero."
else:
    print "Success.  No errors."
