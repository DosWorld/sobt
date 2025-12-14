(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE In;

(*{ #include <stdio.h> *)

VAR Done*: BOOLEAN;

PROCEDURE Open*;
BEGIN
    Done := FALSE;
END Open;

PROCEDURE Char*(p : POINTER TO CHAR);
BEGIN
(*{
*In_p = getc();
*)
END Char;

(*
PROCEDURE Int*(p : POINTER TO INTEGER);
BEGIN
END Int;

PROCEDURE LongInt*(p : POINTER TO LONGINT);
BEGIN
END LongInt;

PROCEDURE Real*(p : POINTER TO REAL);
BEGIN
END Real;

PROCEDURE LongReal*(p : POINTER TO LONGREAL);
BEGIN
END LongReal;

PROCEDURE String*(p : POINTER TO CHAR);
BEGIN
END String;

PROCEDURE Name*(p : POINTER TO CHAR);
BEGIN
END Name;
*)

END In.
