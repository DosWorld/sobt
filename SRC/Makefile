all: sobt.exe sobt32.exe
	copy sobt.exe ..\BIN
	copy sobt32.exe ..\BIN

test.c: sobt.exe
        ./sobt.exe test.mod

sobt.exe: sobt.c
        wcl -ml sobt.c -fe=sobt.exe

sobt32.exe: sobt.c
        wcl386 sobt.c -bt=nt -bcl=nt -fe=sobt32.exe
