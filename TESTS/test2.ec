#include "Out.h"
#include "test2.h"

#define	Test2_C1	15

static char is_Test2_init = 0;
void mod_Test2_init() {
if(is_Test2_init) {
return;
}
is_Test2_init = 1;
Out_String((("Hello, world!")));
Out_Ln();
}
