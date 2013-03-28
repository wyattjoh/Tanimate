all: tanimate

tanimate: tanimate.c
	cc tanimate.c -lcurses -lpthread -o tanimate

clean:
	rm -rf tanimate