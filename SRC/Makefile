CC=gcc
FLAGS=

all: sobt.exe

test: sobt.exe
	./sobt.exe test.mod
sobt.exe: sobt.c
	$(CC) $(FLAGS) sobt.c -o sobt.exe
