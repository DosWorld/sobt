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
