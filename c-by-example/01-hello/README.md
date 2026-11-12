# 01 · Hello

```bash
cd c-by-example/01-hello
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined hello.c -o hello
./hello one two
# Hello, World!
# called with 2 argument(s)
#   argv[1] = one
#   argv[2] = two
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

## The program

[`hello.c`](hello.c) has three ideas:

- **`main` is where it starts.** `argc` counts the words on the command line,
  and `argv` holds them, `argv[0]` being the program's own name.
- **`printf` formats text.** `%d` is an `int`, `%s` a string. The type after
  `%` must match the argument; `-Wall` checks that for you.
- **`return 0` means success.** The shell sees the number:

  ```bash
  ./hello; echo $?      # 0
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

  Every library function belongs to a header. `man 3 printf` names it.

## Exercises

1. Make the program return 3 and check `echo $?`. Then try
   `./hello && echo worked`.
2. Print `argv[0]`. Run it as `./hello` and as `$PWD/hello`.
3. Pass `%d` a string instead of a number. What does `-Wall` say?
