# 00 · Tools

## One time set up

Install [`jbx`](https://just-buildit.github.io/just-bashit/) which gets
you a working dev machine in one command.

```bash
. <(curl -sSL https://just-buildit.github.io/get-jb.sh)
```

Clone this repository and move into the project root

```bash
git clone https://github.com/hunterdsp/examples.git
cd examples
```

On macOS, run `xcode-select --install` first: it brings `cc` (clang),
`make` and `lldb`.

You need three things: a C compiler, `make`, and a debugger. One line
installs them, along with a few other tools that help later on (a second
compiler, a memory checker, CMake, Doxygen), using your system package
manager:

```bash
jbx install-deps -g dev c-by-example/bootstrap.toml
```

Every lesson starts from the repo root, the `examples/` you just cloned
into: its first block `cd`s into the lesson, and the blocks after it run
there.

Check it worked:

```bash
cc --version
# cc (Ubuntu 15.2.0-16ubuntu1) 15.2.0
# Copyright (C) 2025 Free Software Foundation, Inc.
# ...
```

`cc` is not a compiler of its own: it is the traditional name for *your
system's* C compiler, so the same command works everywhere. On Linux it is
usually GCC (the Free Software Foundation line above gives it away; your
version may differ), and on macOS it is Apple's clang. This course uses
`cc` so you never have to care which, and checks every example with both:

```bash
gcc --version     # GCC by name
clang --version   # clang by name (installed above)
```

## What `cc` actually does

```bash
cd c-by-example/00-tools
cc stages.c -o stages && ./stages
# compiled, linked, ran
```

That one command is four programs in a row. Each one can be stopped and
looked at, and knowing which one is complaining is half of fixing any build
error.

**1. Preprocess** (`-E`): text in, text out. `#include` pastes in a file,
`#define` replaces names. Look at the tail of the output: `GREETING` is gone,
replaced by the string it stood for.

```bash
cc -E stages.c | tail -6
```

```c
int
main (void)
{
  puts ("compiled, linked, ran");
  return 0;
}
```

**2. Compile** (`-S`): C becomes assembly for your CPU. The call to `puts`
is right there:

```bash
cc -S stages.c -o - | grep puts
#       call    puts@PLT
```

**3. Assemble** (`-c`): assembly becomes an object file, machine code with
holes in it. `nm` lists what it defines (`T main`) and what it still needs
from somewhere else (`U puts`, undefined):

```bash
cc -c stages.c && nm stages.o
# 0000000000000000 T main
#                  U puts
```

**4. Link**: the linker fills every hole. `puts` comes from the C standard
library, which is linked in by default:

```bash
cc stages.o -o stages && ./stages
```

## Pitfalls

- **"undefined reference to …"** is the *linker* talking (stage 4), not the
  compiler. The code compiled fine; a function it calls was never defined,
  or its library was not linked. Lesson 08 shows this on purpose.
- **A missing `#include`** shows up at stage 2, as "implicit declaration of
  function". Lesson 01 shows that one.

## Exercises

1. Run `cc -E stages.c | wc -l`. Where did all those lines come from?
2. Delete `#include <stdio.h>` and build. Which stage complains?
3. Rename `main` to `start` and build. Which stage complains now, and why
   that one?
