MODULE TextFile;

(*#
#include <stdio.h>
#include <stdlib.h>
*)

PROCEDURE Open*(name : POINTER TO CHAR): POINTER;
BEGIN
(*# return fopen(name, "rw"); *)
END Open;

PROCEDURE OpenNew*(name : POINTER TO CHAR): POINTER;
BEGIN
(*# return fopen(name, "w"); *)
END OpenNew;

PROCEDURE Close*(h : POINTER);
BEGIN
(*# fclose(h); *)
END Close;

PROCEDURE ReadChar*(h : POINTER) : CHAR;
BEGIN
(*# return fgetc(h); *)
END ReadChar;

PROCEDURE WriteChar*(h : POINTER; c : CHAR);
BEGIN
(*# fputc(c, h); *)
END WriteChar;

PROCEDURE Eof*(h : POINTER) : BOOLEAN;
BEGIN
(*# return feof((FILE * )h); *)
END Eof;


BEGIN
END TextFile.