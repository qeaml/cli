# qmlCli

A simple CLI parsing library.

## Usage

```c
#include "cli.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  qmlCliParse(argc, argv);

  // check the amount of positional arguments
  if(qmlCliPosC() >= 1) {
    // get the value of a positional argument
    printf("Hello, %s!\n", qmlCliPos(0));
    // check if a flag is present, in this case -bye or --bye
    if(qmlCliFlag("bye")) {
      printf("Bye, %s!\n", qmlCliPos(0));
    }
    // get the value of a parameter
    char *surname = qmlCliParam("surname");
    // which may be unspecified, hence we have to check it first
    if(surname != NULL) {
      printf("Your surname is %s.\n", surname);
    }
  } else {
    // get the program name from arguments
    printf("Usage: %s <name>\n", qmlCliProgramName());
  }
}
```

## Installation

1. Copy and paste [`cli.c`](cli.c) and [`cli.h`](cli.h) into your project. It
   should work with any C99-compliant compiler.
2. Use [`build_static.sh`](build_static.sh) or
   [`build_shared.sh`](build_shared.sh) to build a static or shared library
   respectively.

## Syntax

Given the following command-line:

```console
greet John --surname=Doe -bye
```

The library will report:

* program name: `greet`
* 1 positional argument: `John`
* 1 flag: `bye`
* 1 parameter: `surname` with value `Doe`

They can be in any order, not necessarily the one shown above or in any of the
test cases seen in the [`test.c`](test.c) file.

Flags can be simply thought of as parameters without values.

The following are all considered to be flags:

```
-some-flag --some-other-flag -----flag-with-many-dashes
```

And the following are all parameters:

```
-param=value --second-param=parameter-value ----p3=with-dashes
```

## License

BSD 3-Clause
