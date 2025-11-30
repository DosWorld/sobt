CC=gcc
CFLAGS   = -std=c99 -O2
CFLAGS  += -Wall -Wextra -Wpedantic -Werror
CFLAGS  += -Wshadow -Wstrict-prototypes -Wmissing-prototypes
CFLAGS  += -Wmisleading-indentation -Wundef -Wconversion
CFLAGS  += -Wwrite-strings -Wunreachable-code

all: sobt.exe

test: sobt.exe
	./sobt.exe test.mod
sobt.exe: sobt.c
	$(CC) $(CFLAGS) sobt.c -o sobt.exe
