(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE FORTH;

IMPORT Out, In, SYSTEM;

CONST
  MEMSIZE = 16000;
  DSSIZE  = 128;

  (* Flags *)
  FIMMED  = 128;
  FHIDDEN = 64;

  (* --- OPCODES --- *)
  OPDOCOL    = 0;
  OPEXIT     = 1;
  OPLIT      = 2;
  OPBRANCH   = 3;
  OPZBRANCH  = 4;
  OPBYE      = 5;

  OPDUP      = 10;
  OPDROP     = 11;
  OPSWAP     = 12;
  OPOVER     = 13;

  OPADD      = 20;
  OPSUB      = 21;
  OPMUL      = 22;
  OPDIV      = 23;
  OPEQ       = 24;
  OPLT       = 25;
  OPGT       = 26;
  OPINV      = 27;
  OPAND      = 28;
  OPOR       = 29;

  OPFETCH    = 30;
  OPSTORE    = 31;
  OPADDSTORE = 32;
  OPCFETCH   = 35;
  OPCSTORE   = 36;

  OPDOT      = 40;
  OPEMIT     = 41;
  OPCR       = 42;
  OPKEY      = 43;

  OPCOMMA    = 50;
  OPCCOMMA   = 51;
  OPHERE     = 52;
  OPLATEST   = 53;
  OPLBRAC    = 54;
  OPRBRAC    = 55;
  OPIMMED    = 56;
  OPHIDDEN   = 57;

  OPSPFETCH  = 60;
  OPSPSTORE  = 61;
  OPRPFETCH  = 62;
  OPRPSTORE  = 63;

  OPRFROM    = 70;
  OPMKCREATE = 71;
  OPDOCREATE = 72;
  OPSETDOES  = 73;
  OPDODOES   = 74;
  OPRFETCH   = 75;

VAR
  MEM: ARRAY MEMSIZE OF INTEGER;

  SP: INTEGER; (* Data Stack Pointer *)
  RP: INTEGER; (* Return Stack Pointer *)

  IP: INTEGER; (* Instruction Pointer *)
  W:  INTEGER; (* Working Pointer *)
  HERE: INTEGER;
  LATEST: INTEGER;
  STATE: INTEGER; (* 0 = Interpret, 1 = Compile *)

  InputBuf: ARRAY 256 OF CHAR;
  InputPos: INTEGER;
  Running: BOOLEAN;

(* --- Helper: String Comparison (No operator overloading in Oberon) --- *)
PROCEDURE StrEquals(s1, s2: POINTER TO CHAR): BOOLEAN;
VAR i: INTEGER;
BEGIN
  i := 0;
  WHILE (s1[i] # 0) & (s2[i] # 0) & (s1[i] = s2[i]) DO INC(i) END;
  RETURN s1[i] = s2[i]
END StrEquals;

(* --- Helper: Bitwise Operations --- *)
PROCEDURE BitAnd(a, b: INTEGER): INTEGER;
BEGIN RETURN SYSTEM.VAL(INTEGER, SYSTEM.VAL(SET, a) * SYSTEM.VAL(SET, b)) END BitAnd;

PROCEDURE BitOr(a, b: INTEGER): INTEGER;
BEGIN RETURN SYSTEM.VAL(INTEGER, SYSTEM.VAL(SET, a) + SYSTEM.VAL(SET, b)) END BitOr;

PROCEDURE BitNot(a: INTEGER): INTEGER;
BEGIN RETURN SYSTEM.VAL(INTEGER, -SYSTEM.VAL(SET, a)) END BitNot;

PROCEDURE BitXor(a, b: INTEGER): INTEGER;
BEGIN RETURN SYSTEM.VAL(INTEGER, SYSTEM.VAL(SET, a) / SYSTEM.VAL(SET, b)) END BitXor;

(* --- Helper: String Length --- *)
PROCEDURE Length(s: POINTER TO CHAR): INTEGER;
VAR i: INTEGER;
BEGIN
  i := 0;
  WHILE s[i] # 0 DO INC(i) END;
  RETURN i
END Length;

(* --- Stack Operations --- *)

PROCEDURE PUSH(Val: INTEGER);
BEGIN
  IF SP > (MEMSIZE - DSSIZE) THEN
    DEC(SP);
    MEM[SP] := Val;
  ELSE
    Out.String("Stack Overflow"); Out.Ln();
    Running := FALSE;
  END
END PUSH;

PROCEDURE POP(): INTEGER;
VAR Res: INTEGER;
BEGIN
  IF SP < MEMSIZE THEN
    Res := MEM[SP];
    INC(SP);
    RETURN Res;
  ELSE
    Out.String("Stack Underflow"); Out.Ln;
    IP := 0;
    RETURN 0;
  END
END POP;

PROCEDURE RPUSH(Val: INTEGER);
BEGIN
  IF RP > HERE + 16 THEN
    DEC(RP);
    MEM[RP] := Val;
  ELSE
    Out.String("Dict/RStack Collision"); Out.Ln;
    Running := FALSE;
  END
END RPUSH;

PROCEDURE RPOP(): INTEGER;
VAR Res: INTEGER;
BEGIN
  IF RP < (MEMSIZE - DSSIZE) THEN
    Res := MEM[RP];
    INC(RP);
    RETURN Res;
  ELSE
    Out.String("RStack Underflow"); Out.Ln;
    IP := 0;
    RETURN 0;
  END
END RPOP;

(* --- Dictionary Management --- *)

PROCEDURE CreatePrimitive(Name: POINTER TO CHAR; OpCode: INTEGER; Immed: BOOLEAN);
VAR Len, i, CharPair: INTEGER;
BEGIN
  MEM[HERE] := LATEST; LATEST := HERE; INC(HERE);
  Len := Length(Name);
  IF Immed THEN Len := BitOr(Len, FIMMED) END;
  MEM[HERE] := Len; INC(HERE);

  i := 0;
  WHILE i < Length(Name) DO
    CharPair := ORD(Name[i]);
    IF i + 1 < Length(Name) THEN
      CharPair := CharPair + (ORD(Name[i+1]) * 256);
    END;
    MEM[HERE] := CharPair; INC(HERE);
    INC(i, 2);
  END;
  MEM[HERE] := OpCode; INC(HERE);
END CreatePrimitive;

PROCEDURE FindWord(Name: POINTER TO CHAR): INTEGER;
VAR Scan, i, CharPair, MaskedLen: INTEGER; Match: BOOLEAN;
BEGIN
  Scan := LATEST;
  WHILE Scan # 0 DO
    MaskedLen := BitAnd(MEM[Scan+1], 31);
    IF MaskedLen = Length(Name) THEN
      Match := TRUE; i := 0;
      WHILE i < Length(Name) DO
        CharPair := ORD(Name[i]);
        IF i + 1 < Length(Name) THEN
          CharPair := CharPair + (ORD(Name[i+1]) * 256);
        END;
        
        IF MEM[Scan+2 + (i DIV 2)] # CharPair THEN
          Match := FALSE; i := Length(Name); (* Break *)
        END;
        INC(i, 2);
      END;
      
      IF Match & (BitAnd(MEM[Scan+1], FHIDDEN) = 0) THEN
        RETURN Scan;
      END;
    END;
    Scan := MEM[Scan];
  END;
  RETURN 0;
END FindWord;

(* --- Input Parser Helpers --- *)

PROCEDURE NextWord(Token: POINTER TO CHAR);
VAR i: INTEGER;
BEGIN
  Token[0] := 0X;
  i := 0;
  (* Skip delimiters *)
  WHILE (InputPos < Length(InputBuf)) & (ORD(InputBuf[InputPos]) <= 32) DO INC(InputPos) END;
  
  (* Copy token *)
  WHILE (InputPos < Length(InputBuf)) & (ORD(InputBuf[InputPos]) > 32) DO
    Token[i] := InputBuf[InputPos];
    INC(i);
    INC(InputPos);
  END;
  Token[i] := 0X;
END NextWord;

PROCEDURE ParseInt(Str: POINTER TO CHAR; Val: POINTER TO INTEGER): BOOLEAN;
VAR i, Sign, Res: INTEGER;
BEGIN
  i := 0; Sign := 1; Res := 0;
  IF Str[0] = '-' THEN Sign := -1; INC(i) END;
  IF Str[i] = 0X THEN RETURN FALSE END; (* Empty or just '-' *)
  
  WHILE Str[i] # 0X DO
    IF (Str[i] >= '0') & (Str[i] <= '9') THEN
      Res := Res * 10 + (ORD(Str[i]) - ORD('0'));
    ELSE
      RETURN FALSE;
    END;
    INC(i);
  END;
  Val[0] := Res * Sign;
  RETURN TRUE;
END ParseInt;

(* --- Virtual Machine --- *)

PROCEDURE ExecutePrimitive(Op: INTEGER);
VAR A, B, Len, CodeAddr: INTEGER; c: CHAR; Token: ARRAY 32 OF CHAR;
BEGIN
  (* NO CASE STATEMENTS - Using IF/ELSIF chain *)

  (* --- Control Flow --- *)
  IF Op = OPBYE THEN
     Running := FALSE;
  ELSIF Op = OPDOCOL THEN
     RPUSH(IP); IP := W + 1;
  ELSIF Op = OPEXIT THEN
     IP := RPOP();
  ELSIF Op = OPLIT THEN
     PUSH(MEM[IP]); INC(IP);
  ELSIF Op = OPBRANCH THEN
     IP := IP + MEM[IP];
  ELSIF Op = OPZBRANCH THEN
     IF POP() = 0 THEN IP := IP + MEM[IP] ELSE INC(IP) END;

  (* --- Stack Ops --- *)
  ELSIF Op = OPDUP THEN
     PUSH(MEM[SP]);
  ELSIF Op = OPDROP THEN
     INC(SP);
  ELSIF Op = OPSWAP THEN
     A:=POP(); B:=POP(); PUSH(A); PUSH(B);
  ELSIF Op = OPOVER THEN
     A:=MEM[SP]; B:=MEM[SP+1]; PUSH(B);

  (* --- Math --- *)
  ELSIF Op = OPADD THEN
     A:=POP(); B:=POP(); PUSH(B+A);
  ELSIF Op = OPSUB THEN
     A:=POP(); B:=POP(); PUSH(B-A);
  ELSIF Op = OPMUL THEN
     A:=POP(); B:=POP(); PUSH(B*A);
  ELSIF Op = OPDIV THEN
     A:=POP(); B:=POP(); PUSH(B DIV A);
  ELSIF Op = OPEQ THEN
     A:=POP(); B:=POP(); IF B=A THEN PUSH(-1) ELSE PUSH(0) END;
  ELSIF Op = OPLT THEN
     A:=POP(); B:=POP(); IF B<A THEN PUSH(-1) ELSE PUSH(0) END;
  ELSIF Op = OPGT THEN
     A:=POP(); B:=POP(); IF B>A THEN PUSH(-1) ELSE PUSH(0) END;
  ELSIF Op = OPINV THEN
     A:=POP(); PUSH(BitNot(A));
  ELSIF Op = OPAND THEN
     A:=POP(); B:=POP(); PUSH(BitAnd(B, A));
  ELSIF Op = OPOR THEN
     A:=POP(); B:=POP(); PUSH(BitOr(B, A));

  (* --- Memory --- *)
  ELSIF Op = OPFETCH THEN
     A:=POP(); PUSH(MEM[A]);
  ELSIF Op = OPSTORE THEN
     A:=POP(); B:=POP(); MEM[A]:=B;
  ELSIF Op = OPADDSTORE THEN
     A:=POP(); B:=POP(); MEM[A]:=MEM[A]+B;
  ELSIF Op = OPCFETCH THEN
     A:=POP(); PUSH(BitAnd(MEM[A], 255));
  ELSIF Op = OPCSTORE THEN
     A:=POP(); B:=POP(); MEM[A] := BitAnd(B, 255);

  (* --- I/O --- *)
  ELSIF Op = OPDOT THEN
     A:=POP(); Out.Int(A, 0); Out.Char(' ');
  ELSIF Op = OPEMIT THEN
     A:=POP(); Out.Char(CHR(A));
  ELSIF Op = OPCR THEN
     Out.Ln;
  ELSIF Op = OPKEY THEN
     In.Char(c); PUSH(ORD(c));

  (* --- Compiler --- *)
  ELSIF Op = OPCOMMA THEN
     MEM[HERE]:=POP(); INC(HERE);
  ELSIF Op = OPCCOMMA THEN
     MEM[HERE]:=BitAnd(POP(), 255); INC(HERE);
  ELSIF Op = OPHERE THEN
     PUSH(HERE);
  ELSIF Op = OPLATEST THEN
     PUSH(LATEST);
  ELSIF Op = OPLBRAC THEN
     STATE := 0;
  ELSIF Op = OPRBRAC THEN
     STATE := 1;
  ELSIF Op = OPIMMED THEN
     MEM[LATEST+1] := BitOr(MEM[LATEST+1], FIMMED);
  ELSIF Op = OPHIDDEN THEN
     MEM[LATEST+1] := BitXor(MEM[LATEST+1], FHIDDEN);

  (* --- Stack Access --- *)
  ELSIF Op = OPSPFETCH THEN
     PUSH(SP);
  ELSIF Op = OPSPSTORE THEN
     SP := POP();
  ELSIF Op = OPRPFETCH THEN
     PUSH(RP);
  ELSIF Op = OPRPSTORE THEN
     RP := POP();
  ELSIF Op = OPRFROM THEN
     PUSH(RPOP());
  ELSIF Op = OPRFETCH THEN
     PUSH(MEM[RP]);

  (* --- Extensions (Create/Does) --- *)
  ELSIF Op = OPMKCREATE THEN
      NextWord(Token);
      CreatePrimitive(Token, OPDOCREATE, FALSE);
      MEM[HERE] := 0; INC(HERE);
  ELSIF Op = OPDOCREATE THEN
      PUSH(W + 2);
  ELSIF Op = OPSETDOES THEN
      A := POP();
      Len := BitAnd(MEM[LATEST+1], 31);
      CodeAddr := LATEST + 2 + ((Len + 1) DIV 2);
      MEM[CodeAddr] := OPDODOES;
      MEM[CodeAddr+1] := A;
  ELSIF Op = OPDODOES THEN
      PUSH(W + 2);
      RPUSH(IP);
      IP := MEM[W + 1];
  END;
END ExecutePrimitive;

PROCEDURE RunInnerLoop;
VAR Code: INTEGER;
BEGIN
  WHILE (IP # 0) & Running DO
    W := MEM[IP];
    INC(IP);
    Code := MEM[W];
    IF Code = OPDOCOL THEN
      RPUSH(IP);
      IP := W + 1;
    ELSE
      ExecutePrimitive(Code);
    END;
  END;
END RunInnerLoop;

PROCEDURE ProcessInput;
VAR
  Token: ARRAY 32 OF CHAR;
  HeaderAddr, CodeAddr, Len, NumVal: INTEGER;
BEGIN
  WHILE InputPos < Length(InputBuf) DO
    NextWord(Token);
    IF Token[0] = 0X THEN InputPos := Length(InputBuf) (* Break *)
    ELSE
      HeaderAddr := FindWord(Token);
      
      IF HeaderAddr # 0 THEN
        Len := BitAnd(MEM[HeaderAddr+1], 31);
        CodeAddr := HeaderAddr + 2 + ((Len + 1) DIV 2);
        
        IF (STATE = 0) OR (BitAnd(MEM[HeaderAddr+1], FIMMED) # 0) THEN
          W := CodeAddr;
          IF MEM[W] = OPDOCOL THEN
             RPUSH(0); IP := W + 1; RunInnerLoop;
          ELSE
             ExecutePrimitive(MEM[W]);
          END;
        ELSE
          MEM[HERE] := CodeAddr;
          INC(HERE);
        END;
        
      ELSE
        IF ParseInt(Token, NumVal) THEN
          IF STATE = 0 THEN PUSH(NumVal)
          ELSE
             (* Compile LIT *)
             HeaderAddr := FindWord("LIT");
             Len := BitAnd(MEM[HeaderAddr+1], 31);
             CodeAddr := HeaderAddr + 2 + ((Len + 1) DIV 2);
             MEM[HERE] := CodeAddr; INC(HERE);
             MEM[HERE] := NumVal;   INC(HERE);
          END;
        ELSIF StrEquals(Token, ":") THEN
          NextWord(Token);
          CreatePrimitive(Token, OPDOCOL, FALSE);
          MEM[LATEST+1] := BitOr(MEM[LATEST+1], FHIDDEN);
          STATE := 1;
        ELSIF StrEquals(Token, ";") THEN
          HeaderAddr := FindWord("EXIT");
          Len := BitAnd(MEM[HeaderAddr+1], 31);
          CodeAddr := HeaderAddr + 2 + ((Len + 1) DIV 2);
          MEM[HERE] := CodeAddr; INC(HERE);
          
          MEM[LATEST+1] := BitXor(MEM[LATEST+1], FHIDDEN);
          STATE := 0;
        ELSE
          Out.String("? "); Out.String(Token); Out.Ln;
          SP := MEMSIZE;
          STATE := 0;
          IP := 0;
          RETURN;
        END;
      END;
    END;
  END;
END ProcessInput;

(* --- Initialization & ReadLine --- *)

PROCEDURE ReadLn(s: POINTER TO CHAR);
VAR c: CHAR; i: INTEGER;
BEGIN
  i := 0;
  REPEAT
    In.Char(c);
    IF (c # $0D) & (c # $0A) & (In.Done) THEN
      IF i < LEN(s) - 1 THEN s[i] := c; INC(i) END;
    END;
  UNTIL (c = $0D) OR (c = $0A) OR (~In.Done);
  s[i] := 0X;
END ReadLn;

PROCEDURE Init;
BEGIN
  HERE := 1;
  LATEST := 0;
  STATE := 0;
  Running := TRUE;
  
  SP := MEMSIZE;
  RP := MEMSIZE - DSSIZE;
  
  CreatePrimitive("EXIT", OPEXIT, FALSE);
  CreatePrimitive("LIT", OPLIT, FALSE);
  CreatePrimitive("BRANCH", OPBRANCH, FALSE);
  CreatePrimitive("0BRANCH", OPZBRANCH, FALSE);
  CreatePrimitive("BYE", OPBYE, FALSE);
  
  CreatePrimitive("DUP", OPDUP, FALSE);
  CreatePrimitive("DROP", OPDROP, FALSE);
  CreatePrimitive("SWAP", OPSWAP, FALSE);
  CreatePrimitive("OVER", OPOVER, FALSE);
  
  CreatePrimitive("+", OPADD, FALSE);
  CreatePrimitive("-", OPSUB, FALSE);
  CreatePrimitive("*", OPMUL, FALSE);
  CreatePrimitive("/", OPDIV, FALSE);
  CreatePrimitive("=", OPEQ, FALSE);
  CreatePrimitive("<", OPLT, FALSE);
  CreatePrimitive(">", OPGT, FALSE);
  CreatePrimitive("INVERT", OPINV, FALSE);
  CreatePrimitive("AND", OPAND, FALSE);
  CreatePrimitive("OR", OPOR, FALSE);
  
  CreatePrimitive("@", OPFETCH, FALSE);
  CreatePrimitive("!", OPSTORE, FALSE);
  CreatePrimitive("+!", OPADDSTORE, FALSE);
  CreatePrimitive("C@", OPCFETCH, FALSE);
  CreatePrimitive("C!", OPCSTORE, FALSE);
  
  CreatePrimitive(".", OPDOT, FALSE);
  CreatePrimitive("EMIT", OPEMIT, FALSE);
  CreatePrimitive("CR", OPCR, FALSE);
  CreatePrimitive("KEY", OPKEY, FALSE);
  
  CreatePrimitive(",", OPCOMMA, FALSE);
  CreatePrimitive("C,", OPCCOMMA, FALSE);
  CreatePrimitive("HERE", OPHERE, FALSE);
  CreatePrimitive("LATEST", OPLATEST, FALSE);
  CreatePrimitive("[", OPLBRAC, TRUE);
  CreatePrimitive("]", OPRBRAC, FALSE);
  CreatePrimitive("IMMEDIATE", OPIMMED, FALSE);
  CreatePrimitive("HIDDEN", OPHIDDEN, FALSE);
  
  CreatePrimitive("SP@", OPSPFETCH, FALSE);
  CreatePrimitive("SP!", OPSPSTORE, FALSE);
  CreatePrimitive("RP@", OPRPFETCH, FALSE);
  CreatePrimitive("RP!", OPRPSTORE, FALSE);
  
  CreatePrimitive("R>", OPRFROM, FALSE);
  CreatePrimitive("R@", OPRFETCH, FALSE);
  
  CreatePrimitive("CREATE", OPMKCREATE, FALSE);
  CreatePrimitive("(DOES>)", OPSETDOES, FALSE);
  
  Out.String("Forth Ready"); Out.Ln;
END Init;

BEGIN
  Init;
  WHILE Running & In.Done DO
    Out.String("ok> ");
    ReadLn(InputBuf);
    InputPos := 0;
    ProcessInput;
  END;
END FORTH.
