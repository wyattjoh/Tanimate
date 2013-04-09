all: tanimate



tanimate: tanimate.c tanimate.h
	cc tanimate.c -lcurses -lpthread -Wall -o tanimate

clean:
	rm -rf tanimate