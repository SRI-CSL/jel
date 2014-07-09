CC = g++

CFLAGS = -Wall  $(shell pkg-config --cflags jel) 

LINK = $(shell pkg-config --libs jel)


VALGRIND = valgrind --leak-check=yes  --dsymutil=yes --trace-children=no  --track-origins=yes


XVALGRIND = valgrind  --leak-check=full --show-reachable=yes --trace-children=no



all: main

images.o: images.cc images.h
	${CC} ${CFLAGS} -c images.cc

main.o: main.cc
	${CC} ${CFLAGS} -c main.cc

main: images.o main.o 
	${CC} images.o main.o -o main  ${LINK} 


clean:
	rm -f main *.o



valgrind: main
	${VALGRIND} ./main


xvalgrind: main
	${XVALGRIND} ./main


