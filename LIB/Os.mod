(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE Os;

(*{
#include <stdio.h>
#include <stdlib.h>

int _argc;
char **_argv;

*)

PROCEDURE ArgC* : INTEGER;
BEGIN
(*{ return _argc; *)
END ArgC;

PROCEDURE Arg*(num : INTEGER) : POINTER TO CHAR;
BEGIN
(*{ return _argv[num]; *)
END Arg;

PROCEDURE Init*(num : INTEGER; p : POINTER);
BEGIN
(*{ _argc = num; _argv = p; *)
END Init;

END Os.