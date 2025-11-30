(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE TextFile;

(*{
#include <stdio.h>
#include <stdlib.h>
*)

PROCEDURE Open*(name : POINTER TO CHAR): POINTER;
BEGIN
(*{ return fopen(name, "rw"); *)
END Open;

PROCEDURE OpenNew*(name : POINTER TO CHAR): POINTER;
BEGIN
(*{ return fopen(name, "w"); *)
END OpenNew;

PROCEDURE Close*(h : POINTER);
BEGIN
(*{ fclose(h); *)
END Close;

PROCEDURE ReadChar*(h : POINTER) : CHAR;
BEGIN
(*{ return fgetc(h); *)
END ReadChar;

PROCEDURE WriteChar*(h : POINTER; c : CHAR);
BEGIN
(*{ fputc(c, h); *)
END WriteChar;

PROCEDURE WriteInt*(h : POINTER; i : INTEGER);
BEGIN
(*{ fprintf(h, "%d", i); *)
END WriteChar;

PROCEDURE WriteStr*(h : POINTER; s : POINTER TO CHAR);
BEGIN
(*{ fprintf(h, "%s", s); *)
END WriteChar;

PROCEDURE WriteLn*(h : POINTER);
BEGIN
(*{ fprintf(h, "\n"); *)
END WriteChar;

PROCEDURE Eof*(h : POINTER) : BOOLEAN;
BEGIN
(*{ return feof((FILE * )h); *)
END Eof;


BEGIN
END TextFile.