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
    
    fmap[k1] += 1
    if (i > 0): emap[k1] = emap[k1] + 1

    return i


#
# Return the Hamming distance between two strings:
#

def hamming(str1, str2):
    if (len(str1) != len(str2)):
        print "Different string lengths?"
        print "Length of string 1 = {:d}".format(len(str1))
        print "Length of string 2 = {:d}".format(len(str2))
        return -1
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
    
    fmap[k1] += 1
    if (i > 0): emap[k1] = emap[k1] + 1

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
#    return sum(itertools.imap(byte_hamming, str1, str2))

#    return sum(itertools.imap(byte_hamming, str1, str2)), len(str1)*8


def hamming_files(file1, file2):
    f1 = open(file1, "rb")
    f2 = open(file2, "rb")

    size1 = os.path.getsize(file1)
    size2 = os.path.getsize(file2)

    nbits, total = hamming(f1.read(), f2.read())

    f1.close()
    f2.close()
    return nbits, total
