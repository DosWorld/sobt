CC=gcc
CFLAGS   = -std=c99 -O2

all: sobt.exe

test: sobt.exe
	./sobt.exe test.mod
sobt.exe: sobt.c
	$(CC) $(CFLAGS) sobt.c -o sobt.exe
