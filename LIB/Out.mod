(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE Out;
(*{ #include <stdio.h> *)

PROCEDURE Open*;
BEGIN
END Open;

PROCEDURE Char*(ch: CHAR);
BEGIN
(*{ printf("%c", Out_ch); *)
END Char;

PROCEDURE String*(str: POINTER TO CHAR);
BEGIN
(*{ printf("%s", Out_str); *)
END String;

PROCEDURE Int*(i, n: LONGINT);
BEGIN
(*{ printf("%d", Out_i); *)
END Int;

(*
PROCEDURE Real*(x: REAL; n: INTEGER);
BEGIN
END Real;

PROCEDURE LongReal*(x: LONGREAL; n: INTEGER);
BEGIN
END LongReal;
*)

PROCEDURE Ln*;
BEGIN
(*{ printf("\n"); *)
END Ln;

END Out.
