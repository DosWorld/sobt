#  <img src="logo.svg" align="right" width="120"> Small Oberon to C99 Translator

A lightweight, single-pass compiler that translates a subset
of the **Oberon** programming language into **C99**.

This project is written in strictly portable C (no structs,
no enums, minimal dependencies) and is designed to be easily
embeddable or used for bootstrapping.

## Features

*   **Output:** Generates paired `.c` (implementation) and `.h` (header) files.
*   **Module System:** Supports `MODULE`, `IMPORT`, and symbol exporting (`*`).
*   **Data Types:**  `BOOLEAN`, `CHAR`, `INTEGER`, `LONGINT`, `POINTER`, and 1D `ARRAY`.
*   **Control Flow:** `IF`/`ELSIF`/`ELSE`, `WHILE`, `REPEAT`/`UNTIL`, `BREAK`, `CONTINUE`.
*   **Built-ins:** `INC`, `DEC`, `SHL`, `SHR`, `Adr` (address of).
*   **C Injection:** Inject raw C code using `(*# ... *)` directives.

## Build

Compile the translator with any standard C compiler:

```bash
gcc -o sobt sobt.c
```

## Usage

Run the translator on your Oberon module file:

```bash
./sobt MyModule.mod
```

This will produce:
*   `MyModule.c`
*   `MyModule.h`

## Example

**Input (`Test.mod`):**
```oberon
MODULE Test;
VAR 
  val*: INTEGER; 

PROCEDURE Add(x: INTEGER);
BEGIN
  INC(val, x)
END Add;

BEGIN
  val := 0;
  Add(10)
END Test.
```

**Generated C (`Test.c`):**
```c
#include "System.h"
#include "Test.h"

int Test_val;

static void Test_Add(int Test_x) {
Test_val += (Test_x);
}

void mod_Test_init() {
Test_val = (0);
Test_Add((10));
}
```

**Generated H (`Test.h`):**
```c
#ifndef Test_H
#define Test_H

#include <stdint.h>

extern int Test_val;

extern void mod_Test_init();
#endif
```

## Limitations

*   **Arrays:** Supports single-dimensional arrays only (`ARRAY 10 OF INTEGER`).
*   **Records:** No support for `RECORD` types (structs).
*   **Syntax:** Strict Oberon subset; expects well-formatted input.

## License

Small Oberon Translator has CC0 1.0 Universal.

