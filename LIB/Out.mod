MODULE Out;

PROCEDURE Open*;
BEGIN
END Open;

PROCEDURE Char*(ch: CHAR);
BEGIN
(*{ printf("%c", ch); *)
END Char;

PROCEDURE String*(str: POINTER TO CHAR);
BEGIN
(*{ printf("%s", str); *)
END String;

PROCEDURE Int*(i, n: LONGINT);
BEGIN
(*{ printf("%d", i); *)
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
