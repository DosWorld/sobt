(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE Os;

(*#
#include <stdio.h>
#include <stdlib.h>

int _argc;
char **_argv;

*)

PROCEDURE ArgC* : INTEGER;
BEGIN
(*# return _argc; *)
END ArgC;

PROCEDURE Arg*(num : INTEGER) : POINTER TO CHAR;
BEGIN
(*# return _argv[num]; *)
END Arg;

PROCEDURE WriteChar*(c : CHAR);
BEGIN
(*# printf("%c", c); *)
END WriteChar;

PROCEDURE WriteInt*(i : INTEGER);
BEGIN
(*# printf("%d", i); *)
END WriteInt;

PROCEDURE WriteStr*(p : POINTER TO CHAR);
BEGIN
(*# printf("%s", p); *)
END WriteStr;

PROCEDURE WriteLn*;
BEGIN
(*# printf("\n"); *)
END WriteLn;

PROCEDURE Init*(num : INTEGER; p : POINTER);
BEGIN
(*# _argc = num; _argv = p; *)
END Init;

BEGIN
END Os.