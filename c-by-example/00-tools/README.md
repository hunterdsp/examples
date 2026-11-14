# 00 · Tools

## One time set up

Get the course, and move into its root folder:

<!-- no-run -->

```bash
git clone https://github.com/hunterdsp/examples.git
cd examples
```

Each lesson is a folder under `c-by-example/`, and its commands run in that
folder: `cd` into it before you start, as this lesson does below. An exercise
answer runs in its own folder, under `exercises/`, and says which.

Part I needs only two tools: a C compiler, `cc`, and `make`, a program that
runs build commands for you (lesson 08 explains it). Many systems already have
both. Check:

```bash
cc --version
make --version
```

If both print a version, you have what Part I needs. On Linux, `cc --version`
prints something like this:

```
cc (Ubuntu 15.2.0-16ubuntu1) 15.2.0
Copyright (C) 2025 Free Software Foundation, Inc.
...
```

`cc` is not a compiler of its own: it is the traditional name for *your
system's* C compiler, so the same command works everywhere. On Linux it is
usually GCC (the Free Software Foundation line above gives it away; your
version may differ), and on macOS it is Apple's clang. This course uses `cc` so
you never have to care which, and checks every example with both.

### If something is missing

Install it with your system's package manager. On macOS:

<!-- no-run -->

```bash
xcode-select --install
```

which brings `cc` (Apple's clang), `make` and the debugger `lldb`. On Debian or
Ubuntu, `sudo apt install build-essential`; on Fedora,
`sudo dnf install gcc make`.

Later parts use more: a second compiler (`clang` on Linux), a debugger, a
memory checker, CMake, Doxygen, pkg-config and Python. Every package the course
uses is listed in [`bootstrap.toml`](../bootstrap.toml), for each package
manager, and you can install them yourself from that list, or let
[`jbx`](https://just-buildit.github.io/just-bashit/) read it and run your
package manager for you. `jbx` installs with a script from the internet, so
read the script first if you like (open its address in a browser), then:

<!-- no-run -->

```bash
. <(curl -sSL https://just-buildit.github.io/get-jb.sh)
jbx install-deps -g dev c-by-example/bootstrap.toml
```

## What `cc` actually does

Here is the program we will take apart. Note the two lines starting with `#`:
they are not C, but instructions to the first stage below.

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

Every C program starts running at `main`. This one does a single thing: `puts`
("put string") prints a line of text. We did not write `puts`; it comes with C,
in the *standard library*, and `#include <stdio.h>` is what tells the compiler
it exists. Keep an eye on it: each stage below changes what `puts` looks like,
and the last one finally finds its code.

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

**1. Preprocess** (`-E`): C in, C out.

`#include` pastes in a whole file and `#define` replaces names, so `stages.i`
is still C, just with every `#` line carried out:

```bash
cc -E stages.c -o stages.i
```

Use `wc` to count the lines before and after preprocessing (your numbers may
differ):

```bash
wc -l stages.c stages.i
#  15 stages.c
# 853 stages.i
# 868 total
```

Print the last 6 lines of the preprocessor output using `tail`:

```bash
tail -6 stages.i
```

The end of `stages.i` is our `main`, with every `#` line carried out:

```c
int
main (void)
{
  puts ("compiled, linked, ran");
  return 0;
}
```

`GREETING` is gone, replaced by the string it stood for, and the hundreds of
lines above `main` are `stdio.h` pasted in, `puts` among them.

**2. Compile** (`-S`): the preprocessed C becomes assembly, the CPU's own
instructions written out as text. Our `puts (...)` line has become one
instruction, `call`, which jumps to `puts` and comes back when it is done:

```bash
cc -S stages.i -o stages.s
grep puts stages.s
#       call    puts@PLT
```

That is GCC on an x86-64 CPU. The exact text depends on your compiler and CPU:
clang writes `callq` and adds a line or two, and an Arm CPU (an Apple Silicon
Mac, say) uses a different instruction altogether. What matters is that `puts`
is still just a name here. The `@PLT` suffix is how a call reaches a function
in a shared library; you can ignore it for now.

**3. Assemble** (`-c`): the assembly becomes an *object file*, machine code
with holes in it. `nm` lists what it defines (`T main`) and what it still needs
from somewhere else (`U puts`, undefined):

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
  compiler. The code compiled fine; a function it calls was never defined, or
  its library was not linked. Lesson 08 shows this on purpose.
- **A missing `#include`** shows up at stage 2, as "implicit declaration of
  function". Lesson 01 shows that one.

## Exercises

Try each one before opening its answer. The output shown is GCC on Linux; clang
words its messages differently but stops at the same stage. (For exercises 2
and 3, [`exercises/`](exercises/) holds the broken program, and `make test`
checks that each still stops where the answer says.)

1. Find `puts` in `stages.i`. Where does the preprocessor say it came from?

   <!-- no-run -->

   ```bash
   grep -n -w puts stages.i
   ```

   <details><summary>Output and answer</summary>

   ```
   716:extern int puts (const char *__s);
   851:  puts ("compiled, linked, ran");
   ```

   Line 716 is the *declaration*: it tells the compiler `puts` takes a string
   and returns an `int`, without saying how it does it. Lines starting
   `# 1 "/usr/include/stdio.h"` near the top of `stages.i` mark where the
   pasted-in file begins. Your line numbers may differ.

   </details>

2. Delete `#include <stdio.h>` and build. Which stage complains?

   <!-- no-run -->

   ```bash
   cc stages.c -o stages && ./stages
   ```

   <details><summary>Output and answer</summary>

   ```
   stages.c: In function ‘main’:
   stages.c:12:3: error: implicit declaration of function ‘puts’ [-Wimplicit-function-declaration]
      12 |   puts (GREETING);
         |   ^~~~
   stages.c:1:1: note: include ‘<stdio.h>’ or provide a declaration of ‘puts’
     +++ |+#include <stdio.h>
       1 | /* stages.c -- the smallest program worth taking apart.
   ```

   The *compiler* (stage 2). The preprocessor is happy: there is simply nothing
   to paste. But without the declaration from exercise 1, the compiler meets
   `puts` with no idea what it is, and refuses to guess. Put the line back
   before going on.

   </details>

3. Rename `main` to `start` and build. Which stage complains now, and why that
   one?

   <!-- no-run -->

   ```bash
   cc stages.c -o stages && ./stages
   ```

   <details><summary>Output and answer</summary>

   ```
   /usr/bin/x86_64-linux-gnu-ld.bfd: /usr/lib/gcc/x86_64-linux-gnu/15/../../../x86_64-linux-gnu/Scrt1.o: in function `_start':
   (.text+0x1b): undefined reference to `main'
   collect2: error: ld returned 1 exit status
   ```

   The *linker* (stage 4): `ld` is the linker. `start` is a perfectly good
   function name, so stages 1 to 3 succeed, and `nm` on the object file shows
   `T start`. But every program is linked with a little startup code from the C
   library (`Scrt1.o` above), and its `_start` calls `main`. Nothing defines
   `main`, so that hole cannot be filled. Rename it back.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                        | header      | what it does                             |
| ----------------------------------------------- | ----------- | ---------------------------------------- |
| [`puts`](https://en.cppreference.com/c/io/puts) | `<stdio.h>` | write a string and a newline to `stdout` |

<!-- /libfuncs -->
