CC = g++

CFLAGS = -Wall  -I../jpeg-6b/  $(shell pkg-config --cflags jel) -I/usr/include/jel

LINK = $(shell pkg-config --libs jel)


VALGRIND = valgrind --leak-check=yes  --dsymutil=yes --trace-children=no


XVALGRIND = valgrind  --leak-check=full --show-reachable=yes --trace-children=no



all: main

images.o: images.cc images.h
	${CC} ${CFLAGS} -c images.cc

main.o: main.cc
	${CC} ${CFLAGS} -c main.cc

main: images.o main.o 
	${CC} images.o main.o -o main  ${LINK} 


clean:
	rm -f *.o



valgrind: main
	${VALGRIND} ./main


xvalgrind: main
	${XVALGRIND} ./main


