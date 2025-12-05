SOBT = ..\bin\sobt64

all: test1 test2

test1: test1.mod
	$(SOBT) test1.mod
	cmp test1.c test1.ec
	cmp test1.h test1.eh

test2: test2.mod
	$(SOBT) test2.mod
	cmp test2.c test2.ec
	cmp test2.h test2.eh

clean:
	del *.h
	del *.c
