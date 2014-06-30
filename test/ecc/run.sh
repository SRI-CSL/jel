#!/bin/csh
set N = 3661
wedge -nolength -data msg.dat test.jpg msg.jpg
unwedge -length $N msg.jpg out.dat
