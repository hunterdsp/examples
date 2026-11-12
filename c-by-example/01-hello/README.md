# 01 · Hello

Run this lesson's commands in `c-by-example/01-hello/`.

A first program, and the build command used for the rest of the course.
Here it is:

```bash
cat hello.c
```

```c
/* hello.c -- a first program, and the build command used from here on.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       hello.c -o hello && ./hello
 */
#include <stdio.h>

int
main (int argc, char *argv[])
{
  printf ("Hello, World!\n");

  /* argv[0] is how the program was invoked; argc counts every word on the
     command line, including that one. */
  printf ("called with %d argument(s)\n", argc - 1);
  for (int i = 1; i < argc; i++)
    printf ("  argv[%d] = %s\n", i, argv[i]);

  /* 0 tells the shell "success". Anything else is a failure code, which
     is how `make`, `&&` and `if` know your program went wrong. */
  return 0;
}
```

Three ideas are in it:

- **`main` is where it starts.** `argc` counts the words on the command line,
  and `argv` holds them, `argv[0]` being the program's own name.
- **`printf` formats text.** `%d` is an `int`, `%s` a string. The type after
  `%` must match the argument; `-Wall`, below, checks that for you.
- **`return 0` means success.** The shell sees the number, as you will see
  once it is built.

## Building it

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined hello.c -o hello
./hello one two
```

```
Hello, World!
called with 2 argument(s)
  argv[1] = one
  argv[2] = two
```

That build command is the one used for the rest of Parts I–II. Every flag is
there on purpose:

| Flag | Why |
|---|---|
| `-std=c99` | The language this course teaches, and nothing the compiler adds on top |
| `-Wall -Wextra` | Warnings are the compiler telling you about a bug for free. Read every one |
| `-g` | Debug info, so errors point at line numbers instead of addresses |
| `-fsanitize=address,undefined` | Stop the program at the *first* out-of-bounds access, use-after-free or undefined behaviour, with the line it happened on |

The sanitizers make the program slower and bigger, so a release build leaves
them off (Part V). While learning, they are the difference between a crash
that says *where* and a wrong answer that says nothing.

## Exit status

`return 0` hands the shell a number, and `$?` holds the last one:

```bash
./hello; echo $?
```

```
Hello, World!
called with 0 argument(s)
0
```

`make`, `&&` and `if` all decide what to do next from this one number, so a
program that fails must return something other than 0.

## Pitfalls

- **Calling a function nobody declared.** [`broken-no-include.c`](broken-no-include.c)
  calls `printf` without `#include <stdio.h>`. Old C guessed what the
  function looked like; C99 refuses, and the build stops:

  ```
  error: implicit declaration of function 'printf' [-Wimplicit-function-declaration]
  ```

  Every library function belongs to a header, `printf` in `stdio.h`.
  `man 3 printf` prints the usage guide.

> [!TIP]
> **Reading a man page.** Every C library function has a manual page, and
> it is the quickest answer to "what do I include, and how do I call it?"
>
> - **The section number matters.** Section 1 is shell commands, 2 system
>   calls, 3 C library functions. `man printf` finds the shell's `printf`
>   first; `man 3 printf` finds the C one.
> - **Read the top.** `SYNOPSIS` gives the `#include` line and the
>   function's declaration; `LIBRARY` gives the link flag, if one is needed
>   (`man 3 sqrt` says `-lm`, which lesson 02 uses).
> - **Move around** with the vi keys: space and `b` page down and up,
>   `/word` searches, `n` jumps to the next match, and `:q` (or just `q`)
>   returns to the terminal prompt.
> - **Don't know the name?** `man -k printf` lists every page whose name or
>   summary mentions it.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says
which folder to run it in.

1. Make the program return 3 and check `echo $?`. Then try
   `./hello && echo worked`.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-exit-status/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined hello.c -o hello
   ./hello; echo $?
   ```

   ```
   Hello, World!
   called with 0 argument(s)
   3
   ```

   `echo $?` prints the status the program returned: 3. Now `&&`:

   ```bash
   ./hello && echo worked
   ```

   ```
   Hello, World!
   called with 0 argument(s)
   ```

   The program ran and printed its greeting, but `worked` is missing:
   `&&` runs its right-hand side only when the left one returns 0. The
   lesson's own program, two folders up, returns 0:

   ```bash
   ../../hello && echo worked
   ```

   ```
   Hello, World!
   called with 0 argument(s)
   worked
   ```

   That one number is how every script and Makefile decides whether to
   carry on.

   </details>

2. Print `argv[0]`. Run the program as `./hello`, and again by a different
   path to the same file. What does `argv[0]` hold?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-argv0/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined hello.c -o hello
   ./hello
   ../ex2-argv0/hello
   ```

   ```
   argv[0] = ./hello
   argv[0] = ../ex2-argv0/hello
   ```

   Exactly what you typed: the same program, two different `argv[0]`s.
   It is the *name it was run by*, not where the file lives, so a program
   cannot rely on it to find files next to itself.

   </details>

3. Pass `%d` a string instead of a number. What does `-Wall` say?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-format/`:

   <!-- no-run -->
   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined broken-format.c -o broken-format
   ./broken-format
   ```

   GCC prints this, then builds the program anyway (clang words it
   differently, and says the same thing):

   ```
   broken-format.c: In function ‘main’:
   broken-format.c:12:13: warning: format ‘%d’ expects argument of type ‘int’, but argument 2 has type ‘char *’ [-Wformat=]
      12 |   printf ("%d\n", "three");
         |            ~^     ~~~~~~~
         |             |     |
         |             int   char *
         |            %s
   ```

   It is only a *warning*, so the program builds, and runs, and prints
   nonsense: a different number every run (`-1068400608` here), because
   `printf` read part of the string's address as if it were an `int`.
   `printf` cannot see its arguments' types; the format is all it goes
   by. That is why the warning is worth stopping for, and why `check.sh`
   builds with `-Werror`, which turns every warning into an error.

   </details>
