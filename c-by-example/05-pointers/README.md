# 05 · Pointers

Run this lesson's commands in `c-by-example/05-pointers/`.

Every variable lives somewhere in memory, at an *address*. A **pointer** is a
variable that holds an address. Two operators are all there is:

|                  |                                                         |
| ---------------- | ------------------------------------------------------- |
| `float *p = &x;` | `p` holds the address of `x`                            |
| `&x`             | the address of `x`                                      |
| `*p`             | the thing `p` points at, here `x`: read it, or write it |

In a program:

```bash
cat address.c
```

```c
/* address.c -- a pointer, and writing through it.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       address.c -o address && ./address
 */
#include <stdio.h>

int
main (void)
{
  float  x = 1.0f;
  float *p = &x; /* p holds the address of x */

  printf ("x is %.1f\n", x);
  printf ("*p is %.1f\n", *p); /* the thing p points at: x */

  *p = 2.0f; /* write through p, and x changes */
  printf ("after *p = 2.0f, x is %.1f\n", x);
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined address.c -o address
./address
```

```
x is 1.0
*p is 1.0
after *p = 2.0f, x is 2.0
```

`p` never held a copy of `x`, only where `x` lives, so writing through `p`
changes `x` itself.

You have used addresses already: lessons 03 and 04 passed arrays to functions,
and what arrived was the address of the first element. A pointer gives that
address a name and a type, so you can keep it, pass it, and step it along. That
is the whole idea. Every buffer, every object in Part III and every library API
in Part V is built on it.

Here is the program:

```bash
cat pointers.c
```

```c
/* pointers.c -- an address is a value you can pass around.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       pointers.c -o pointers && ./pointers
 *
 * A pointer holds the address of something. `&x` is x's address; `*p` is
 * whatever p points at. That is the whole idea, and everything in Parts
 * II and III is built on it.
 */
#include <stddef.h>
#include <stdio.h>

/* Lesson 03 could not change the caller's variable. With its ADDRESS, a
   function can: it writes where the variable lives. */
static void
swap (float *a, float *b)
{
  float tmp = *a;
  *a        = *b;
  *b        = tmp;
}

/* In place: read and write the same buffer. `buf` is a pointer to the
   first sample, and `n` is how many follow (an array arrives as exactly
   this, lesson 04). */
static void
gain_in_place (float *buf, size_t n, float g)
{
  for (size_t i = 0; i < n; i++)
    buf[i] *= g; /* buf[i] is shorthand for *(buf + i) */
}

/* Out of place, with the contract in the types:
     const float *in   -- this function will not write the input
     float *out        -- it will write the output
     restrict          -- in and out never overlap, so the compiler may
                          keep values in registers (lesson 26) */
static void
gain (const float *restrict in, float *restrict out, size_t n, float g)
{
  for (size_t i = 0; i < n; i++)
    out[i] = in[i] * g;
}

/* Pointer arithmetic: p + 1 is the NEXT float, 4 bytes on, not 1 byte.
   A loop can walk a pointer instead of an index. Returns NULL (a pointer
   to nothing) when there is no peak to report. */
static const float *
find_peak (const float *buf, size_t n)
{
  if (n == 0)
    return NULL;
  const float *peak = buf;
  for (const float *p = buf + 1; p < buf + n; p++)
    if (*p > *peak)
      peak = p;
  return peak;
}

int
main (void)
{
  float x = 1.0f, y = 2.0f;
  swap (&x, &y);
  printf ("after swap: x = %.1f, y = %.1f\n", x, y);

  float  sig[] = { 0.5f, -1.0f, 2.0f, 0.3f };
  size_t n     = sizeof sig / sizeof sig[0];

  float half[4];
  gain (sig, half, n, 0.5f);
  gain_in_place (sig, n, 2.0f);
  printf ("doubled:");
  for (size_t i = 0; i < n; i++)
    printf (" %.2f", sig[i]);
  printf ("\nhalved: ");
  for (size_t i = 0; i < n; i++)
    printf (" %.2f", half[i]);
  printf ("\n");

  const float *peak = find_peak (sig, n);
  /* Subtracting two pointers into one array gives the distance between
     them, in elements: here, the peak's index. */
  printf ("peak %.2f at index %td\n", *peak, peak - sig);

  if (find_peak (sig, 0) == NULL)
    printf ("empty buffer: no peak (NULL)\n");

  printf ("sig[1] lives %td bytes after sig[0]\n",
          (const char *)&sig[1] - (const char *)&sig[0]);
  return 0;
}
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined pointers.c -o pointers
./pointers
```

```
after swap: x = 2.0, y = 1.0
doubled: 1.00 -2.00 4.00 0.60
halved:  0.25 -0.50 1.00 0.15
peak 4.00 at index 2
empty buffer: no peak (NULL)
sig[1] lives 4 bytes after sig[0]
```

## What it shows

- **`swap` changes the caller's variables.** Lesson 03 couldn't, because
  arguments are copies. A copy of an *address* still leads to the original, so
  `*a = *b` writes where the caller's `x` lives.
- **A buffer is a pointer and a length.**
  `gain_in_place(float *buf, size_t n, …)` is the shape of nearly every DSP
  function you will ever write. `buf[i]` is shorthand for `*(buf + i)`.
- **The types state the contract.** In `gain(const float *in, float *out, …)`,
  the `const` promises the input comes back untouched, and the compiler
  enforces it. `restrict` promises `in` and `out` don't overlap, which frees
  the compiler to optimise (lesson 26).
- **Pointer arithmetic counts elements, not bytes.** `p + 1` is the next
  `float`, 4 bytes on. Subtracting two pointers into the same array gives the
  distance in elements, which is how `find_peak`'s result becomes an index.
- **`NULL` means "points at nothing".** `find_peak` returns it for an empty
  buffer, and the caller checks before using the result.

## Pitfalls

- **Following a NULL pointer.** [`broken-null.c`](broken-null.c) forgets the
  check. Address 0 is never valid, so the program dies, and the sanitizer names
  the line:

  ```
  broken-null.c:25:26: runtime error: load of null pointer of type 'const float'
  ```

  Without the sanitizers it is a plain `Segmentation fault`, with no line
  number at all.

  If a function can return `NULL`, every caller checks. If it never returns
  `NULL`, say so in its documentation, so nobody wastes a check.

- **Writing through `const`.** [`broken-const.c`](broken-const.c) "tidies up"
  its input. The build stops (gcc: "assignment of read-only location"; clang:
  "read-only variable is not assignable"). Use `const` on every pointer a
  function only reads: it's documentation the compiler checks.

- **Pointers into dead memory.** A pointer stays valid only as long as the
  thing it points at. Lesson 09 returns a pointer to a local variable, which is
  gone the moment the function returns.

- **Uninitialised pointers.** `float *p; *p = 1;` writes to a random address.
  Initialise every pointer, to `NULL` if to nothing else.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Write `void min_max(const float *buf, size_t n, float *lo, float *hi)` that
   hands back two results through pointers.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-min-max/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined min_max.c -o min_max
   ./min_max
   ```

   ```
   min -1.00, max 2.00
   ```

   The caller passes `&lo` and `&hi`, and
   [`min_max`](exercises/ex1-min-max/min_max.c) writes through them, as `swap`
   did. The types tell the story: `const float *buf` is read, `float *lo` and
   `float *hi` are written. It needs `n` to be at least 1, since an empty
   buffer has no minimum; say so where it is declared, as the code does.

   </details>

2. Rewrite `gain_in_place` to walk a pointer (`for (float *p = buf; …)`)
   instead of an index. Check the output is identical.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-pointer-walk/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined walk.c -o walk
   ./walk
   ```

   ```
   doubled: 1.00 -2.00 4.00 0.60
   identical to the index version: yes
   ```

   The loop is `for (float *p = buf; p < buf + n; p++) *p *= g;`. `buf + n`
   points one past the last sample: C lets you form that address and compare
   against it, but never read through it. An optimising compiler turns both
   versions into the same machine code, so pick whichever reads better.

   </details>

3. Print `sizeof(float *)` and `sizeof(char *)`. Why are they the same?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-pointer-size/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sizes.c -o sizes
   ./sizes
   ```

   ```
   sizeof(float *)  = 8
   sizeof(char *)   = 8
   sizeof(double *) = 8
   but sizeof(float) = 4 and sizeof(char) = 1
   ```

   A pointer holds an address, and an address is the same size whatever lives
   there: 8 bytes on a 64-bit machine. The pointed-to type does not change the
   pointer; it tells the compiler how many bytes to read through it, and how
   far `p + 1` steps.

   </details>
