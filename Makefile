CC=gcc
CFLAGS=-Wall -lncurses -pthread
RFLAGS=
OUT=snake

all: compile

compile: main.c
	$(CC) $(CFLAGS) -o $(OUT) $^
	$(CC) $(CFLAGS) -g -o $(OUT)-debug $^

run:
	./$(OUT) $(RFLAGS)

debug:
	gdb --args $(OUT) $(RFLAGS)
