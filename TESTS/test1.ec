#include "test1.h"

int Test_val;
static int Test_n;

static void Test_Add(int Test_x) {
int Test_d;
Test_val += ((Test_x) + (2 * 4));
}

static char is_Test_init = 0;
void mod_Test_init() {
if(is_Test_init) {
return;
}
is_Test_init = 1;
Test_val = ((0));
Test_Add(((10)));
}
