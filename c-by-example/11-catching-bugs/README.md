# 11 · Catching bugs

Run this lesson's commands in `c-by-example/11-catching-bugs/`.

Every lesson has built with the same flags since lesson 01, and some of the
broken examples have shown what they buy. This lesson is about the flags
themselves: what each checker catches, and what none of them do.

C does not check what you do with memory. Read past the end of an array, use a
block after `free`, overflow an `int`: the language calls all of these
*undefined behaviour*, and a compiled program usually carries on, wrong. The
four checkers in the course's build command turn those silent wrong answers
into a stop at the line that caused them:

| checker                                | turned on by              | catches                                                               |
| -------------------------------------- | ------------------------- | --------------------------------------------------------------------- |
| the compiler's warnings                | `-Wall -Wextra`           | what it can see without running: unused, uninitialised, wrong formats |
| **AddressSanitizer** (ASan)            | `-fsanitize=address`      | out-of-bounds access, use after `free`, double `free`, leaks (Linux)  |
| **UndefinedBehaviorSanitizer** (UBSan) | `-fsanitize=undefined`    | signed overflow, bad shifts, `NULL` access, out-of-range indexes      |
| your own `assert`s                     | always, unless `-DNDEBUG` | whatever *you* know must be true                                      |

Here is a program that states its assumptions with `assert`:

```bash
cat checked.c
```

```c
/* checked.c -- a program that states its assumptions, so tools can check
 * them.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       checked.c -o checked && ./checked
 *
 * Built like this, four checkers watch every run: the compiler's warnings
 * (-Wall -Wextra), AddressSanitizer (bad memory accesses and leaks),
 * UndefinedBehaviorSanitizer (overflow, bad shifts, out-of-bounds
 * indexes) and the program's own assert()s. This one passes all four;
 * the broken-*.c files beside it each fail one.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* The mean of n samples. An empty buffer has no mean, so n == 0 is the
   CALLER's bug, not a case to handle: assert says so, in code, and stops
   the program with the file and line if it ever happens. */
static float
mean (const float *buf, size_t n)
{
  assert (buf != NULL);
  assert (n > 0);
  double sum = 0.0;
  for (size_t i = 0; i < n; i++)
    sum += (double)buf[i];
  return (float)(sum / (double)n);
}

int
main (void)
{
#ifdef NDEBUG
  printf ("asserts: off (built with -DNDEBUG)\n");
#else
  printf ("asserts: on\n");
#endif

  size_t n   = 8;
  float *buf = malloc (n * sizeof *buf);
  if (buf == NULL)
    return 1;
  for (size_t i = 0; i < n; i++)
    buf[i] = (float)i; /* 0 1 2 ... 7: every element written, none past */
  printf ("mean of 0..7 = %.2f\n", mean (buf, n));
  free (buf); /* freed once, and never touched again */
  printf ("no checker complained\n");
  return 0;
}
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined checked.c -o checked
./checked
```

```
asserts: on
mean of 0..7 = 3.50
no checker complained
```

A *release* build turns the asserts off with `-DNDEBUG`, and drops the
sanitizers, which make a program two to three times slower:

```bash
cc -std=c99 -Wall -Wextra -O2 -DNDEBUG checked.c -o checked
./checked
```

```
asserts: off (built with -DNDEBUG)
mean of 0..7 = 3.50
no checker complained
```

## What it shows

- **`assert(condition)` states a fact the code relies on.** If it is ever
  false, the program stops at once and names the file, line and condition. It
  is for the *programmer's* mistakes, such as asking for the mean of nothing,
  not for things that can go wrong in a correct program, such as a missing
  file. Those get a return value and a message.
- **Asserts cost nothing in a release build.** `-DNDEBUG` defines `NDEBUG`, and
  `<assert.h>` then makes every `assert` vanish. So an assert must never do
  work the program needs: `assert(fread(...) == n)` would skip the read in a
  release build.
- **Clean under the checkers is the bar.** `checked.c` writes every element and
  none past, frees once, and never touches freed memory. The sanitizers only
  see what *runs*, so the bar is "clean on every path you can run": the reason
  lesson 31 tests each one.

## Pitfalls

Each of these broken programs fails one checker. `make test` builds each with
the course's flags and checks it fails for the reason given here.

- **Use after free.** [`broken-use-after-free.c`](broken-use-after-free.c)
  frees a buffer, then reads it. gcc sees it while compiling ("pointer 'buf'
  may be used after 'free'"); clang leaves it to AddressSanitizer:

  ```
  ERROR: AddressSanitizer: heap-use-after-free on address 0x780940fe0010 at pc 0x605fec463b7f bp 0x7fff87211370 sp 0x7fff87211368
  ```

  Without ASan this usually *works*: the samples are often still there. Until
  another `malloc` reuses the block.

- **Heap overflow.** [`broken-heap-overflow.c`](broken-heap-overflow.c) is
  lesson 04's off-by-one on a `malloc`ed block. clang's ASan reports a
  `heap-buffer-overflow`; gcc's UBSan reaches the same write first:

  ```
  broken-heap-overflow.c:21:12: runtime error: store to address 0x76c0765e0060 with insufficient space for an object of type 'float'
  ```

- **Signed overflow.** [`broken-signed-overflow.c`](broken-signed-overflow.c)
  sums 70,000 full-scale `int16_t` samples into an `int32_t`, which needs 33
  bits (lesson 02's table):

  ```
  broken-signed-overflow.c:26:24: runtime error: signed integer overflow: 2147483646 + 32767 cannot be represented in type 'int32_t' (aka 'int')
  ```

  Accumulate into an `int64_t`, casting both operands.

- **A broken assumption.** [`broken-assert.c`](broken-assert.c) asks for the
  mean of an empty buffer, and `mean`'s assert stops it (the wording differs by
  C library; this is Linux's):

  ```
  broken-assert: broken-assert.c:15: mean: Assertion `n > 0' failed.
  ```

- **Uninitialised variables.** [`broken-uninit.c`](broken-uninit.c) adds to a
  `sum` that never started at 0. Neither sanitizer here checks this, but the
  compiler does (gcc: "'sum' is used uninitialized"; clang: "variable 'sum' is
  uninitialized when used here").

- **The checkers see only what runs.** A bug on a path no run takes stays
  hidden, sanitizers or not. And none of them work on code built without them:
  a library compiled elsewhere is not checked inside.

- **Not every platform has every check.** LeakSanitizer runs by default on
  Linux, not with Apple's compiler on macOS; there, `leaks --atExit -- ./prog`
  does the job. Valgrind (`valgrind ./prog`, Linux) finds many of the same bugs
  in a program built *without* sanitizers, at fifty times the run time.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. [`leaky.c`](exercises/ex1-leak/leaky.c) allocates four blocks and frees
   none. Build it, read the leak report, and fix it.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-leak/`:

   <!-- no-run -->

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined leaky.c -o leaky
   ./leaky
   ```

   On Linux, when the program exits (trimmed):

   ```
   ERROR: LeakSanitizer: detected memory leaks

   Direct leak of 4096 byte(s) in 4 object(s) allocated from:
       #0 0x7cad8d92b40f in calloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:74
       #1 0x58e33fa8f229 in block_new …/11-catching-bugs/exercises/ex1-leak/leaky.c:16
       #2 0x58e33fa8f24b in main …/11-catching-bugs/exercises/ex1-leak/leaky.c:24
   ```

   Read the stack from the top: `calloc` was called by `block_new` at line 16,
   which was called by `main` at line 24. Four blocks of 1024 bytes, one per
   pass of the loop, and nothing freed them. `block_new`'s comment says the
   caller must free, so the fix goes in `main`, before `block` goes out of
   scope: [`fixed.c`](exercises/ex1-leak/fixed.c).

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined fixed.c -o fixed
   ./fixed
   ```

   ```
   block 0: 1024 bytes
   block 1: 1024 bytes
   block 2: 1024 bytes
   block 3: 1024 bytes
   ```

   </details>

2. Build a 32-bit mask with `1 << bit` for `bit` = 31. What does UBSan say, and
   how do you build the mask safely?

   <details><summary>Output and answer</summary>

   [`broken-shift.c`](exercises/ex2-shift/broken-shift.c) stops at the shift:

   ```
   broken-shift.c:15:16: runtime error: left shift of 1 by 31 places cannot be represented in type 'int'
   ```

   `1` is an `int`, and 2³¹ is one more than the largest `int`: signed overflow
   by another name. Shift an unsigned 32-bit 1 instead.

   In `exercises/ex2-shift/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined shift.c -o shift
   ./shift
   ```

   ```
   mask 0x80000000 (bit 31)
   ```

   [`shift.c`](exercises/ex2-shift/shift.c) writes `(uint32_t)1 << bit`. Masks
   and bit fields are unsigned, always. Shifting by 32 or more is still
   undefined, for every type: the count must be less than the width.

   </details>

3. Lesson 06 writes each `float` as 4 bytes. Make the build *fail* on any
   machine where a `float` isn't 4 bytes.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-compile-time/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sizes.c -o sizes
   ./sizes
   ```

   ```
   float is 4 bytes, double 8: checked when compiling
   ```

   [`sizes.c`](exercises/ex3-compile-time/sizes.c) declares an array type whose
   size is 1 when the condition holds, and −1 when it doesn't, which no
   compiler accepts. Change a `4` to a `5` and the build stops: gcc says "size
   of array 'static_check_float_is_4_bytes' is negative". That is an assert
   checked while *compiling*, before there is a program to run. C11 spells it
   `_Static_assert`; this course is C99, so it uses the trick.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                | header       | what it does                                                    |
| ------------------------------------------------------- | ------------ | --------------------------------------------------------------- |
| [`assert`](https://en.cppreference.com/c/error/assert)  | `<assert.h>` | stop the program if a condition is false; removed by `-DNDEBUG` |
| [`calloc`](https://en.cppreference.com/c/memory/calloc) | `<stdlib.h>` | allocate memory for n elements, all bytes zero                  |
| [`free`](https://en.cppreference.com/c/memory/free)     | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc`           |
| [`malloc`](https://en.cppreference.com/c/memory/malloc) | `<stdlib.h>` | allocate memory, contents unset                                 |
| [`printf`](https://en.cppreference.com/c/io/fprintf)    | `<stdio.h>`  | formatted output to `stdout`                                    |

<!-- /libfuncs -->
