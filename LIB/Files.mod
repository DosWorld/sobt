MODULE Files;

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
BEGIN
END Files.