CC=gcc
CFLAGS=-Wall -lncurses -pthread
RFLAGS=
OL=2
OUT=snake

all: compile

compile: main.c
	$(CC) $(CFLAGS) -O$(OL) -o $(OUT) $^
	$(CC) $(CFLAGS) -g -o $(OUT)-debug $^

run:
	./$(OUT) $(RFLAGS)

debug:
	gdb --args $(OUT) $(RFLAGS)
