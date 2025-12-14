(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE Strings;

(*{
#include <string.h>
*)

(*

Length(s) returns the number of characters in s up to and excluding the first 0X.
Insert(src, pos, dst) inserts the string src into the string dst at position pos (0 <=pos<=Length(dst)). If pos = Length(dst),src is appended to dst. If the size of dst is not large enough to hold the result of the operation, the result is truncated so that dst is always terminated with a 0X.
Append(s,dst) has the same effect as Insert(s,Length(dst),dst).
Delete(s, pos, n) deletes n characters from s starting at position pos (0 <= pos œ Length(s)). If n > Length(s) - pos, the new length of s is pos.
Replace(src, pos, dst) has the same effect as Delete(dst, pos, Length(src)) followed by an Insert(src, pos, dst).
Extract(src, pos, n, dst) extracts a substring dst with n characters from position pos (0 <= pos œ Length(src)) in src. If n > Length(src) - pos, dst is only the part of src from pos to the end of src, i.e. Length(src) -1. If the size of dst is not large enough to hold the result of the operation, the result is truncated so that dst is always terminated with a 0X.
Pos(pat, s, pos) returns the position of the first occurrence of pat in s. Searching starts at position pos. If pat is not found, -1 is returned.
Cap(s) replaces each lower case letter within s by its upper case equivalent.

*)
PROCEDURE Length*(s: POINTER TO CHAR): INTEGER;
BEGIN
(*{ return strlen(Strings_s); *)
END Length;

PROCEDURE Append*(extra: POINTER TO CHAR; dest: POINTER TO CHAR);
BEGIN
(*{ strcat(Strings_dest, Strings_extra); *)
END Append;

PROCEDURE Compare*(s1 , s2: POINTER TO CHAR) : INTEGER;
BEGIN
(*{ return strcmp(Strings_s1, Strings_s2); *)
END Compare;

PROCEDURE Copy*(source: POINTER TO CHAR; dest: POINTER TO CHAR);
BEGIN
(*{ strcpy(Strings_dest, Strings_source); *)
END Copy;

(*

PROCEDURE Insert*(source: POINTER TO CHAR; pos: INTEGER;  dest: POINTER TO CHAR);
BEGIN
END Insert;

PROCEDURE Delete*(s: POINTER TO CHAR; pos, n: INTEGER);
BEGIN
END Delete;

PROCEDURE Replace*(source: POINTER TO CHAR; pos: INTEGER; dest: POINTER TO CHAR);
BEGIN
END Replace;

PROCEDURE Extract*(source: POINTER TO CHAR;  pos, n: INTEGER;  dest: POINTER TO CHAR);
BEGIN
END Extract;

PROCEDURE Pos*(pattern, s: POINTER TO CHAR;  pos: INTEGER): INTEGER;
BEGIN
END Pos;

PROCEDURE Cap*(s: POINTER TO CHAR);
BEGIN
END Cap;

*)

END Strings.
