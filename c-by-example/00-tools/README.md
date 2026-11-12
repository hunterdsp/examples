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

Here is the program we will take apart. Note the two lines starting with
`#`: they are not C, but instructions to the first stage below.

```bash
cd c-by-example/00-tools
cat stages.c
```

```c
/* stages.c -- the smallest program worth taking apart.
 *
 * Build it in one step:      cc stages.c -o stages && ./stages
 * Or watch each stage:       see README.md
 */
#include <stdio.h>

#define GREETING "compiled, linked, ran"

int
main (void)
{
  puts (GREETING);
  return 0;
}
```

Every C program starts running at `main`. This one does a single thing:
`puts` ("put string") prints a line of text. We did not write `puts`; it
comes with C, in the *standard library*, and `#include <stdio.h>` is what
tells the compiler it exists. Keep an eye on it: each stage below changes
what `puts` looks like, and the last one finally finds its code.

Build and run it in one step:

```bash
cc stages.c -o stages && ./stages
# compiled, linked, ran
```

That one command is four programs in a row, each reading the file the one
before it wrote. Run them one at a time and you get every file in between,
which you can open and read. Knowing which stage is complaining is half of
fixing any build error.

```
stages.c  --preprocess-->  stages.i  --compile-->  stages.s
          --assemble---->  stages.o  --link----->  stages
```

**1. Preprocess** (`-E`): C in, C out. `#include` pastes in a whole file and
`#define` replaces names, so `stages.i` is still C, just with every `#` line
carried out:

```bash
cc -E stages.c -o stages.i
wc -l stages.c stages.i     # a few lines in, hundreds out: stdio.h
tail -6 stages.i
```

```c
int
main (void)
{
  puts ("compiled, linked, ran");
  return 0;
}
```

`GREETING` is gone, replaced by the string it stood for, and the hundreds
of lines above `main` are `stdio.h` pasted in, `puts` among them.

**2. Compile** (`-S`): the preprocessed C becomes assembly, the CPU's own
instructions written out as text. Our `puts (...)` line has become one
instruction, `call`, which jumps to `puts` and comes back when it is done:

```bash
cc -S stages.i -o stages.s
grep puts stages.s
#       call    puts@PLT
```

That is GCC on an x86-64 CPU. The exact text depends on your compiler and
CPU: clang writes `callq` and adds a line or two, and an Arm CPU (an Apple
Silicon Mac, say) uses a different instruction altogether. What matters is
that `puts` is still just a name here. The `@PLT` suffix is how a call
reaches a function in a shared library; you can ignore it for now.

**3. Assemble** (`-c`): the assembly becomes an *object file*, machine code
with holes in it. `nm` lists what it defines (`T main`) and what it still
needs from somewhere else (`U puts`, undefined):

```bash
cc -c stages.s -o stages.o
nm stages.o
# 0000000000000000 T main
#                  U puts
```

**4. Link**: the linker fills every hole and writes a program you can run.
`puts` comes from the C standard library, which is linked in by default:

```bash
cc stages.o -o stages
./stages
# compiled, linked, ran
```

Once you know the stages exist, you rarely need to run them by hand:
`cc -save-temps stages.c -o stages` does all four in one go and keeps every
file in between.

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
