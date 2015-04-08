#!/bin/bash -x
# Make sure we exit if there is a failure
set -e

#!/bin/bash -x
# Make sure we exit if there is a failure
set -e

export CC=gcc

autoreconf -i
./configure --enable-silent-rules

make clean
make
