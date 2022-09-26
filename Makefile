CC=gcc
CFLAGS=-Wall -lncurses -pthread
RFLAGS=
OL=2
OUT=snake

all: compile

compile: main.c
	$(CC) -O$(OL) -o $(OUT) $^ $(CFLAGS)
	$(CC) -g -o $(OUT)-debug $^ $(CFLAGS)

run:
	./$(OUT) $(RFLAGS)

debug:
	gdb --args $(OUT) $(RFLAGS)
