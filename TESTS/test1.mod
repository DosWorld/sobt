MODULE Test;
VAR 
  val*: INTEGER; 
n : INTEGER;

PROCEDURE Add(x: INTEGER);
VAR d : INTEGER;
BEGIN
  INC(val, x + 2 * 4)
END Add;

BEGIN
  val := 0;
  Add(10)
END Test.
