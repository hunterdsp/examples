# 15 · Function pointers

Run this lesson's commands in `c-by-example/15-function-pointers/`.

A function's code sits in memory like everything else, so it has an address,
and C lets you keep that address in a variable and call whatever it points at.
That is a **function pointer**, and it is how C code says "do *this*", where
*this* is chosen at run time: which filter, which comparison, what to call when
a block of audio arrives.

The syntax is the hardest part, so start with a name for the type:

```c
typedef float (*shaper_fn) (float x);
```

Read it from the name outward: `shaper_fn` is a pointer (`*`) to a function
taking `(float x)` and returning `float`. Any function with that signature can
be stored in a `shaper_fn`. The parentheses around `*shaper_fn` matter: without
them, it would declare a function returning a `float *`.

Here are three waveshapers, the curves a distortion pedal pushes a signal
through, chosen from a table:

```bash
cat shape.c
```

```c
/* shape.c -- waveshapers, chosen from a table of function pointers.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       shape.c -o shape && ./shape
 *
 * A waveshaper maps each sample through a curve: it is how a guitar
 * pedal distorts. Every curve here has the same signature, float in and
 * float out, so a pointer to any of them has the same type, shaper_fn,
 * and one loop can apply whichever it is handed.
 */
#include <stdio.h>

/* The type of a pointer to a function taking a float and returning one.
   Read it inside out: shaper_fn is a pointer (*) to a function (float)
   returning float. */
typedef float (*shaper_fn) (float x);

/* Hard clip: anything past +-1 is cut flat. */
static float
hard (float x)
{
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return x;
}

/* Soft clip: a cubic that bends smoothly into +-1 instead of cutting. */
static float
soft (float x)
{
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return 1.5f * x - 0.5f * x * x * x;
}

/* Fold: past +-1 the signal is reflected back, like a mirror. */
static float
fold (float x)
{
  if (x > 1.0f)
    return 2.0f - x;
  if (x < -1.0f)
    return -2.0f - x;
  return x;
}

/* Apply f to every sample of buf, in place. apply doesn't know or care
   which curve f is: it calls whatever function the pointer points at. */
static void
apply (float *buf, size_t n, shaper_fn f)
{
  for (size_t i = 0; i < n; i++)
    buf[i] = f (buf[i]);
}

/* A table of names and functions: adding a shaper is adding a row. A
   function's name, used without (), is its address. */
static const struct
{
  const char *name;
  shaper_fn   fn;
} shapers[] = {
  { "hard", hard },
  { "soft", soft },
  { "fold", fold },
};

enum
{
  N = 9
};

int
main (void)
{
  float x[N];
  for (int i = 0; i < N; i++)
    x[i] = (float)(i - 4) * 0.5f; /* -2.0, -1.5, ..., 2.0 */

  printf ("in  ");
  for (int i = 0; i < N; i++)
    printf (" %5.2f", (double)x[i]);
  printf ("\n");

  size_t count = sizeof shapers / sizeof shapers[0];
  for (size_t k = 0; k < count; k++)
    {
      float buf[N];
      for (int i = 0; i < N; i++)
        buf[i] = x[i];
      apply (buf, N, shapers[k].fn);
      printf ("%s", shapers[k].name);
      for (int i = 0; i < N; i++)
        printf (" %5.2f", (double)buf[i]);
      printf ("\n");
    }

  shaper_fn f = soft; /* a variable like any other: assign, compare, call */
  printf ("f == soft: %s; f(0.5) = %.4f\n", f == soft ? "yes" : "no",
          (double)f (0.5f));
  return 0;
}
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined shape.c -o shape
./shape
```

```
in   -2.00 -1.50 -1.00 -0.50  0.00  0.50  1.00  1.50  2.00
hard -1.00 -1.00 -1.00 -0.50  0.00  0.50  1.00  1.00  1.00
soft -1.00 -1.00 -1.00 -0.69  0.00  0.69  1.00  1.00  1.00
fold  0.00 -0.50 -1.00 -0.50  0.00  0.50  1.00  0.50  0.00
f == soft: yes; f(0.5) = 0.6875
```

Inside ±1, `hard` and `fold` pass the signal untouched and `soft` already bends
it (0.5 comes out as 0.69); beyond ±1, each does something different with what
doesn't fit.

## Passing state along: `void *`

`apply` needs nothing but the function. Most callbacks need somewhere to keep
what they find, and the code calling them can't know what that is. So it takes
a `void *`, and passes it to the callback untouched. A `void *` is an address
with no type attached: any object pointer converts to one and back, without a
cast. (`malloc` returns one, which is why its result can be stored straight
into a `float *`.)

```bash
cat blocks.c
```

```c
/* blocks.c -- a callback, and the void * that carries its state.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       blocks.c -o blocks && ./blocks
 *
 * for_each_block cuts a signal into blocks and calls a function on each:
 * a CALLBACK. Most callbacks need somewhere to keep what they find, and
 * for_each_block can't know what that is, so it passes along a void *
 * it was given, untouched. The caller knows what it points at; the
 * callback turns it back into that.
 */
#include <stdio.h>

/* The callback's type: a block, its length, and the caller's pointer. */
typedef void (*block_fn) (const float *blk, size_t n, void *user);

/* Call fn on each block of size samples (the last may be shorter),
   handing it user every time. */
static void
for_each_block (const float *buf, size_t n, size_t size, block_fn fn,
                void *user)
{
  for (size_t start = 0; start < n; start += size)
    {
      size_t len = n - start < size ? n - start : size;
      fn (buf + start, len, user);
    }
}

/* One callback: the peak of each block, appended to a log. */
typedef struct
{
  float  peak[8];
  size_t count;
} peak_log;

static void
log_peak (const float *blk, size_t n, void *user)
{
  peak_log *log  = user; /* we passed a peak_log *, so that is what it is */
  float     peak = 0.0f;
  for (size_t i = 0; i < n; i++)
    {
      float a = blk[i] < 0.0f ? -blk[i] : blk[i];
      if (a > peak)
        peak = a;
    }
  if (log->count < sizeof log->peak / sizeof log->peak[0])
    log->peak[log->count++] = peak;
}

/* Another: count samples beyond a limit. Different state, same type. */
typedef struct
{
  float  limit;
  size_t over;
} clip_count;

static void
count_clips (const float *blk, size_t n, void *user)
{
  clip_count *c = user;
  for (size_t i = 0; i < n; i++)
    if (blk[i] > c->limit || blk[i] < -c->limit)
      c->over++;
}

enum
{
  N = 20
};

int
main (void)
{
  /* A sawtooth: 0.0 to 0.6 in steps of 0.1, then twice as loud. */
  float buf[N];
  for (size_t i = 0; i < N; i++)
    buf[i] = (float)(i % (size_t)7) * (i < (size_t)7 ? 0.1f : 0.2f);

  peak_log log = { { 0.0f }, 0 };
  for_each_block (buf, N, 8, log_peak, &log);
  printf ("peaks of %zu blocks:", log.count);
  for (size_t b = 0; b < log.count; b++)
    printf (" %.1f", (double)log.peak[b]);
  printf ("\n");

  clip_count clips = { 1.0f, 0 };
  for_each_block (buf, N, 8, count_clips, &clips);
  printf ("samples beyond %.1f: %zu\n", (double)clips.limit, clips.over);
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined blocks.c -o blocks
./blocks
```

```
peaks of 3 blocks: 0.6 1.2 1.0
samples beyond 1.0: 1
```

`for_each_block` was written once and serves both: it never learns that one
caller's pointer is a `peak_log` and the other's a `clip_count`.

## What it shows

- **A function's name is its address.** The row `{ "hard", hard }` stores where
  `hard`'s code is; `f (x)` calls whatever `f` points at. (`&hard` and
  `(*f) (x)` mean the same, and older code writes them that way.)
- **Same signature, same type.** `hard`, `soft` and `fold` all take and return
  a `float`, so one type holds any of them, and one `apply` uses any of them.
- **Tables of functions.** Pairing names with functions in an array turns a
  choice into a lookup. Adding a shaper is one function and one row, and
  nothing else changes: `count` comes from `sizeof`, not from a number someone
  has to remember to update (exercise 1).
- **`void *user` carries the callback's state.** The code that calls back
  passes the pointer along and never looks at it. The callback knows what it
  really is, and converts it back: `peak_log *log = user;`.
- **This is half of an object.** A function pointer is behaviour chosen at run
  time; the `void *` is the data it works on. Put the two together in one
  struct and you have lesson 16.

## Pitfalls

- **A function that doesn't fit the type.**
  [`broken-wrong-signature.c`](broken-wrong-signature.c) stores a function
  taking and returning `double` in a `shaper_fn`. Called through it, `half`
  would be handed a `float` where it reads a `double`. The compiler refuses:

  ```
  broken-wrong-signature.c:23:17: error: initialization of ‘shaper_fn’ {aka ‘float (*)(float)’} from incompatible pointer type ‘double (*)(double)’ [-Wincompatible-pointer-types]
  ```

  A cast, `(shaper_fn)half`, makes the error go away and the bug stay: calling
  a function through a pointer of the wrong type is undefined behaviour. Write
  a small function with the right signature that calls `half` instead.

- **Calling `NULL`.** A function pointer that was never set, or a lookup that
  found nothing, is `NULL`, and calling it crashes. Check before calling, as
  exercise 3 does.

- **The `void *` lies if you let it.** Pass a `clip_count *` to `log_peak` and
  the compiler can't object: both go through `void *`. The callback reads the
  wrong struct, and nothing reports it. Keep each callback next to the struct
  it expects, and name them alike.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Add a half-wave rectifier, which turns negative samples into 0, to
   `shape.c`'s table. What else has to change?

   <details><summary>Output and answer</summary>

   In `exercises/ex1-rectify/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined shape.c -o shape
   ./shape
   ```

   ```
   in   -2.00 -1.50 -1.00 -0.50  0.00  0.50  1.00  1.50  2.00
   hard -1.00 -1.00 -1.00 -0.50  0.00  0.50  1.00  1.00  1.00
   soft -1.00 -1.00 -1.00 -0.69  0.00  0.69  1.00  1.00  1.00
   fold  0.00 -0.50 -1.00 -0.50  0.00  0.50  1.00  0.50  0.00
   rect  0.00  0.00  0.00  0.00  0.00  0.50  1.00  1.50  2.00
   f == soft: yes; f(0.5) = 0.6875
   ```

   Nothing else. [`shape.c`](exercises/ex1-rectify/shape.c) gains `rectify` and
   the row `{ "rect", rectify }`; `main` loops over however many rows the table
   has, because it divides `sizeof shapers` by the size of one row.

   </details>

2. The standard library's `qsort` sorts an array of anything, given a function
   that compares two elements. Use it to find the median of a block with a
   click in it. Why is the median the better measure here, and why not write
   the comparison as `return x - y;`?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-median/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined median.c -o median
   ./median
   ```

   ```
   sorted:  0.10 0.10 0.15 0.20 0.20 0.25 9.00
   mean 1.43, median 0.20
   x - y compares 0.10 with 0.15: 0, "equal"
   ```

   One click drags the mean to 1.43, seven times bigger than any other sample;
   the median, the middle of the sorted block, ignores it. A median filter
   removes clicks this way.

   `qsort` hands the comparator two `const void *`, since it doesn't know the
   element type, and [`cmp_float`](exercises/ex2-median/median.c) converts them
   back to `const float *`. It must return a negative, zero or positive `int`.
   `x - y` is a `float`, and converting −0.05 to `int` gives 0, so `qsort`
   would be told that different samples are equal, and sort them wrongly.
   `(x > y) - (x < y)` is −1, 0 or 1, exactly.

   </details>

3. Write a program that takes a shaper's name and a value on the command line,
   `./pick soft 0.5`, and prints the result. What should it do with a name that
   isn't in the table?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-by-name/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined pick.c -o pick
   ./pick soft 0.5
   ./pick fold 1.5
   ./pick fuzz 0.5
   ```

   ```
   soft(0.50) = 0.6875
   fold(1.50) = 0.5000
   unknown shaper 'fuzz'; try: hard soft fold
   ```

   [`find`](exercises/ex3-by-name/pick.c) searches the table with `strcmp` and
   returns the matching function, or `NULL` when there is none. `main` checks
   for `NULL` before calling, says what the choices are, and exits with status

   1. Calling the `NULL` would crash, with nothing to say which name was wrong.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                               |
| ------------------------------------------------------------ | ------------ | ---------------------------------------------------------- |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf)        | `<stdio.h>`  | `printf` to a stream, such as `stderr`                     |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                               |
| [`qsort`](https://en.cppreference.com/c/algorithm/qsort)     | `<stdlib.h>` | sort an array, given a function that compares two elements |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp) | `<string.h>` | compare two strings                                        |
| [`strtod`](https://en.cppreference.com/c/string/byte/strtof) | `<stdlib.h>` | read a `double` from a string                              |

<!-- /libfuncs -->
