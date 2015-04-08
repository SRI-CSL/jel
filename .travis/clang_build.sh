#!/bin/bash -x
# Make sure we exit if there is a failure
set -e

export CC=clang

autoreconf -i
./configure --enable-silent-rules

make clean
make


