(*  (c) by DosWorld is marked CC0 1.0 Universal.
    To view a copy of this mark,
    visit https://creativecommons.org/publicdomain/zero/1.0/  *)

MODULE Hello;

IMPORT Os;

PROCEDURE sayhello;
BEGIN
      Os.WriteStr("Hello, world!\n");
END sayhello;


BEGIN
       sayhello();
END Hello.

(*#
int main(int argc, char **argv) {
  mod_Hello_init();
  return 0;
}
*)
