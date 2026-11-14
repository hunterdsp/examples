# 03 · Functions

Run this lesson's commands in `c-by-example/03-functions/`.

A sine wave is the atom of signal processing: every signal is a sum of them
(lesson 24 proves it). This lesson builds a table of one, and in doing so
splits a program across files, the way every C library is built.

The header, `sine.h`, declares what `sine.c` offers:

```bash
cat sine.h
```

```c
/* sine.h -- the PROMISE: what sine.c offers, and nothing about how.
 *
 * Any file that calls these includes this header, so the compiler can
 * check every call against the declaration. The include guard (#ifndef)
 * makes including it twice harmless; lesson 08 says more.
 */
#ifndef SINE_H
#define SINE_H

#include <stddef.h> /* size_t */

/* Fill out[0..n-1] with `cycles` periods of a unit sine wave. */
void sine_table (float out[], size_t n, double cycles);

/* How many tables sine_table has filled so far. */
int sine_tables_made (void);

#endif /* SINE_H */
```

`sine.c` holds the code:

```bash
cat sine.c
```

```c
/* sine.c -- the IMPLEMENTATION of sine.h. */
#include "sine.h"

#include <math.h>

/* M_PI is POSIX, not C99, so -std=c99 does not provide it: define our own. */
static const double TWO_PI = 6.283185307179586;

/* `static` at file scope means PRIVATE: visible in this file only. Nothing
   outside can reach `made` except through sine_tables_made(). */
static int made = 0;

void
sine_table (float out[], size_t n, double cycles)
{
  for (size_t i = 0; i < n; i++)
    out[i] = (float)sin (TWO_PI * cycles * (double)i / (double)n);
  made++;
}

int
sine_tables_made (void)
{
  return made;
}
```

And `table.c` calls it:

```bash
cat table.c
```

```c
/* table.c -- calling functions, and what "pass by value" means.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       table.c sine.c -o table -lm && ./table
 */
#include <stdio.h>

#include "sine.h"

/* Arguments are COPIES. `x` here is a new variable holding the caller's
   value; changing it changes nothing outside. (Lesson 05 shows how a
   function changes its caller's data: pass where the data lives.) */
static void
try_to_double (int x)
{
  x = x * 2;
  printf ("  inside:  x = %d\n", x);
}

/* An array argument is passed as the address of its first element, so
   the function CAN write into the caller's array; `const` promises this
   one won't. More in lesson 04. */
static void
print_table (const char *label, const float table[], size_t n)
{
  printf ("%s:", label);
  for (size_t i = 0; i < n; i++)
    printf (" %7.4f", table[i]);
  printf ("\n");
}

/* `static` INSIDE a function means something else: the variable is made
   once, when the program starts, and keeps its value between calls. A
   plain local is made afresh on every call. */
static void
count_calls (void)
{
  static int kept  = 0;
  int        fresh = 0;
  kept++;
  fresh++;
  printf ("  kept = %d, fresh = %d\n", kept, fresh);
}

int
main (void)
{
  int n = 21;
  try_to_double (n);
  printf ("  outside: n = %d\n", n);

  float one[8], two[8];
  sine_table (one, 8, 1.0); /* one period over 8 samples */
  sine_table (two, 8, 2.0); /* two periods: twice the frequency */
  print_table ("1 cycle ", one, 8);
  print_table ("2 cycles", two, 8);
  printf ("tables made: %d\n", sine_tables_made ());

  count_calls ();
  count_calls ();
  count_calls ();
  return 0;
}
```

Build both `.c` files into one program, and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined table.c sine.c -o table -lm
./table
```

```
  inside:  x = 42
  outside: n = 21
1 cycle :  0.0000  0.7071  1.0000  0.7071  0.0000 -0.7071 -1.0000 -0.7071
2 cycles:  0.0000  1.0000  0.0000 -1.0000 -0.0000  1.0000  0.0000 -1.0000
tables made: 2
  kept = 1, fresh = 1
  kept = 2, fresh = 1
  kept = 3, fresh = 1
```

## A header is a promise

| File                 | Holds                                                                   |
| -------------------- | ----------------------------------------------------------------------- |
| [`sine.h`](sine.h)   | **Declarations**: the name, arguments and return type of each function. |
| [`sine.c`](sine.c)   | **Definitions**: the code.                                              |
| [`table.c`](table.c) | A caller. It includes `sine.h`, so the compiler checks every call.      |

The compiler builds `sine.c` and `table.c` separately, and the linker joins
them (lesson 00). The header is how the two agree on what a function looks like
without either seeing the other's code.

## What it shows

- **Pass by value.** `try_to_double(n)` doubles its own *copy*: 42 inside,
  still 21 outside. A C function never changes the caller's variables through
  its arguments, unless you hand it an address (lesson 05).
- **Arrays are different.** `sine_table(one, 8, 1.0)` fills the caller's array.
  An array argument is passed as the address of its first element, so the
  function writes straight into it.
- **`static` means two things, depending on where it is.** At file scope it
  makes a name private to its file: `made` in `sine.c` is invisible to every
  other file, and only `sine_tables_made()` can read it. The same goes for
  functions: `try_to_double` and `print_table` are `static`, so no other file
  can call them. This is the first step toward the objects of Part III. Inside
  a function, `static` changes how long a variable lives instead: in
  `count_calls`, `kept` is made once and keeps counting across calls, while the
  plain local `fresh` is made anew each time and is always 1.
- **`const` is a promise not to write.** `print_table` takes
  `const float table[]`, and the compiler holds it to that.

## Two periods, twice the frequency

The `2 cycles` row fits two sine periods into the same 8 samples, so it repeats
every 4: twice the frequency. Push `cycles` up to 4 and every sample lands on a
zero crossing: the table is all zeros, and the wave has vanished. Push it past
4 and 5 cycles gives the same samples as 3 cycles, upside down: the samples can
no longer tell which frequency they came from. That limit, half the sample
rate, is the **Nyquist frequency**, and lesson 20 comes back to it.

## Why `-0.0000`?

sin(2π) should be 0, but 2π has no exact binary form (lesson 02), so the result
is −2.4×10⁻¹⁶, and `%.4f` keeps its sign. Floating-point results are *nearly*
right, never exactly. Plan for it.

## Pitfalls

- **Declaring a function differently in two files.** If `table.c` declared
  `sine_table` itself, with the wrong arguments, both files would compile and
  link, and the program would quietly pass garbage. Declare a function in
  exactly one header and include it everywhere, *including* in its own `.c`
  file, so the compiler checks the definition against the promise.
- **`M_PI` is not standard C.** It's POSIX; under `-std=c99` it is not defined.
  `sine.c` defines its own constant.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Add `void sine_scale(float table[], size_t n, float gain)` to `sine.h` and
   `sine.c`, and use it to halve a table.

   <details><summary>Output and answer</summary>

   The declaration goes in [`sine.h`](exercises/ex1-sine-scale/sine.h), the
   code in [`sine.c`](exercises/ex1-sine-scale/sine.c), and
   [`halve.c`](exercises/ex1-sine-scale/halve.c) calls it:

   In `exercises/ex1-sine-scale/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined halve.c sine.c -o halve -lm
   ./halve
   ```

   ```
   full:  0.0000  0.7071  1.0000  0.7071  0.0000 -0.7071 -1.0000 -0.7071
   half:  0.0000  0.3536  0.5000  0.3536  0.0000 -0.3536 -0.5000 -0.3536
   ```

   `sine_scale` changes the caller's samples, not a copy, because an array
   argument arrives as the address of its first element.

   </details>

2. Remove `static` from `made` and declare `extern int made;` in `table.c`. It
   works. Why is that worse?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-extern/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined table.c sine.c -o table -lm
   ./table
   ```

   ```
   tables made: 2
   tables made: -5 (according to sine_tables_made: -5)
   ```

   It works: `table.c` reads the count directly. But it can also *write* it,
   and so can every other file in the program, so the count is only as right as
   the least careful of them. Even `sine_tables_made()` now reports −5. With
   `static`, only `sine.c` can change `made`, and a wrong count can only be
   `sine.c`'s fault.

   </details>

3. Generate 3.5 cycles over 8 samples and print it. Where does the
   discontinuity land, and what would it sound like on repeat?

   <details><summary>Output and answer</summary>

   [`cycles.c`](exercises/ex3-half-cycle/cycles.c) builds against the lesson's
   own `sine.c`, two folders up (`-I ../..` tells the compiler to look there
   for `sine.h` too). It prints the table played twice (`looped`), under the
   same sine carried on for 16 samples (`wave`):

   In `exercises/ex3-half-cycle/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. cycles.c ../../sine.c -o cycles -lm
   ./cycles
   ```

   ```
   wave  :  0.00  0.38 -0.71  0.92 -1.00  0.92 -0.71  0.38  0.00 -0.38  0.71 -0.92  1.00 -0.92  0.71 -0.38
   looped:  0.00  0.38 -0.71  0.92 -1.00  0.92 -0.71  0.38  0.00  0.38 -0.71  0.92 -1.00  0.92 -0.71  0.38
   ```

   They agree up to sample 8, where the second copy starts. There the wave is
   half-way through a cycle and heading down, but the loop starts the table
   again, heading up: from then on it is the wave upside down. That seam comes
   round every 8 samples, so instead of a pure tone, a loop of this table plays
   a harsh, buzzy one. Loop only tables that hold a whole number of cycles.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                | header      | what it does                 |
| ------------------------------------------------------- | ----------- | ---------------------------- |
| [`printf`](https://en.cppreference.com/c/io/fprintf)    | `<stdio.h>` | formatted output to `stdout` |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin) | `<math.h>`  | sine of an angle in radians  |

<!-- /libfuncs -->
