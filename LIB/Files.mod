(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE Files;

(*{ #include <stdio.h> *)

PROCEDURE Old*(name: POINTER TO CHAR): POINTER;
BEGIN
(*{ return fopen(Files_name, "rb"); *)
END Old;

PROCEDURE New*(name: POINTER TO CHAR): POINTER;
BEGIN
(*{ return fopen(Files_name, "rwb"); *)
END New;

PROCEDURE BlockRead*(f : POINTER; buf : POINTER; count : INTEGER): INTEGER;
BEGIN
(*{  return fread(Files_buf, 1, Files_count, Files_f); *)
END BlockRead;

PROCEDURE BlockWrite*(f : POINTER; buf : POINTER; count : INTEGER): INTEGER;
BEGIN
(*{  return fwrite(Files_buf, 1, Files_count, Files_f); *)
END BlockWrite;

PROCEDURE Close*(f: POINTER);
BEGIN
(*{ fclose(Files_f); *)
END Close;


PROCEDURE Seek*(f : POINTER; ofs : LONGINT);
BEGIN
(*{  fseek(f, ofs, SEEK_SET); *)
END Seek;


PROCEDURE Pos*(f : POINTER) : LONGINT;
BEGIN
(*{  return ftell(f); *)
END Pos;

PROCEDURE Eof*(f : POINTER) : BOOLEAN;
BEGIN
(*{  return feof(f); *)
END Pos;

(*
PROCEDURE Old (name: ARRAY OF CHAR): File;
PROCEDURE New (name: ARRAY OF CHAR): File;
PROCEDURE Register (f: File);
PROCEDURE Close (f: File);
PROCEDURE Purge (f: File);
PROCEDURE Delete (name: ARRAY OF CHAR; VAR res: INTEGER);
PROCEDURE Rename (old, new: ARRAY OF CHAR; 
			  VAR res: INTEGER);
PROCEDURE Length (f: File): LONGINT;
PROCEDURE GetDate (f: File; VAR t, d: LONGINT);
PROCEDURE Set (VAR r: Rider; f: File; pos: LONGINT);
PROCEDURE Pos (VAR r: Rider): LONGINT;
PROCEDURE Base (VAR r: Rider): File;
PROCEDURE Read (VAR r: Rider; VAR x: SYSTEM.BYTE);
PROCEDURE ReadInt (VAR R: Rider; VAR x: INTEGER);
PROCEDURE ReadLInt (VAR R: Rider; VAR x: LONGINT);
PROCEDURE ReadReal (VAR R: Rider; VAR x: REAL);
PROCEDURE ReadLReal (VAR R: Rider; VAR x: LONGREAL);
PROCEDURE ReadNum (VAR R: Rider; VAR x: LONGINT);
PROCEDURE ReadString (VAR R: Rider; VAR x: ARRAY OF CHAR);
PROCEDURE ReadSet (VAR R: Rider; VAR x: SET);
PROCEDURE ReadBool (VAR R: Rider; VAR x: BOOLEAN;
PROCEDURE ReadBytes (VAR r: Rider;
			     VAR x: ARRAY OF SYSTEM.BYTE;
 			     n: LONGINT);
PROCEDURE Write (VAR r: Rider; x: SYSTEM.BYTE); 
PROCEDURE WriteInt (VAR R: Rider; x: INTEGER);
PROCEDURE WriteLInt (VAR R: Rider; x: LONGINT);
PROCEDURE WriteReal (VAR R: Rider; x: REAL);
PROCEDURE WriteLReal (VAR R: Rider; x: LONGREAL);
PROCEDURE WriteNum (VAR R: Rider; x: LONGINT);
PROCEDURE WriteString (VAR R: Rider; x: ARRAY OF CHAR);
PROCEDURE WriteSet (VAR R: Rider; x: SET);
PROCEDURE WriteBool (VAR R: Rider; x: BOOLEAN);
PROCEDURE WriteBytes (VAR r: Rider; 
			      VAR x: ARRAY OF SYSTEM.BYTE; 
			      n: LONGINT)
*)

END Files.