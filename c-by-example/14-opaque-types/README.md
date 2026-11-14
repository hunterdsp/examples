# 14 · Opaque types

Run this lesson's commands in `c-by-example/14-opaque-types/`.

Part III builds objects out of plain C: data, the functions that work on it,
and a wall between the two and everyone else. C has no `class` and no
`private`, but it has something that does the same job: a struct the compiler
knows *exists*, without knowing what is *in* it.

```c
typedef struct lowpass lowpass_t; /* declared, never defined here */
```

A file that sees only this line may hold a `lowpass_t *`, pass it around and
hand it back, but cannot create a `lowpass_t`, copy one, or look inside one:
the compiler doesn't know its size or its members. That is an **opaque type**,
and it makes the functions in the header the *only* way in.

Here is a one-pole low-pass filter written that way. Each output moves the
fraction `a` of the way from the last output toward the input, which smooths
out sudden changes. The header, `lowpass.h`, is all a caller sees:

```bash
cat lowpass.h
```

```c
/* lowpass.h -- a one-pole low-pass filter, as an OPAQUE type.
 *
 * The header names the type but never says what is inside it:
 *
 *   typedef struct lowpass lowpass_t;
 *
 * declares `struct lowpass` without defining it. A file that includes
 * this header may hold a lowpass_t * and pass it around, but cannot
 * create one, copy one, or look inside one: only lowpass.c, which has
 * the definition, can. So the functions below are the ONLY way in, and
 * lowpass.c is free to change what the struct holds without breaking a
 * single caller.
 */
#ifndef LOWPASS_H
#define LOWPASS_H

#include <stddef.h>

typedef struct lowpass lowpass_t; /* declared here, defined in lowpass.c */

/* Make a filter with smoothing coefficient a, 0 < a <= 1: each output
   moves the fraction a of the way from the last output to the input.
   Small a smooths more. The caller owns the result and must pass it to
   lowpass_destroy. Returns NULL if a is out of range or memory is out. */
lowpass_t *lowpass_create (float a);

/* Free the filter. Accepts NULL. */
void lowpass_destroy (lowpass_t *f);

/* Filter n samples of buf in place. The filter remembers its last output,
   so a signal processed in blocks gives the same result as in one go. */
void lowpass_process (lowpass_t *f, float *buf, size_t n);

/* Forget the past: the next output starts again from 0. */
void lowpass_reset (lowpass_t *f);

#endif /* LOWPASS_H */
```

`lowpass.c` is the one file with the definition:

```bash
cat lowpass.c
```

```c
/* lowpass.c -- the only file that knows what a lowpass_t holds. */
#include "lowpass.h"

#include <stdlib.h>

/* The definition the header leaves out. Change it, and only this file
   needs recompiling: no caller ever saw it. */
struct lowpass
{
  float a; /* smoothing coefficient, 0 < a <= 1 */
  float y; /* the last output: the filter's memory */
};

lowpass_t *
lowpass_create (float a)
{
  if (!(a > 0.0f && a <= 1.0f)) /* written so a NaN fails it too */
    return NULL;
  lowpass_t *f = malloc (sizeof *f);
  if (f == NULL)
    return NULL;
  f->a = a;
  f->y = 0.0f;
  return f;
}

void
lowpass_destroy (lowpass_t *f)
{
  free (f); /* free(NULL) does nothing, so neither does this */
}

void
lowpass_process (lowpass_t *f, float *buf, size_t n)
{
  const float a = f->a; /* local copies: the loop needn't touch *f */
  float       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * (buf[i] - y);
      buf[i] = y;
    }
  f->y = y; /* remembered for the next block */
}

void
lowpass_reset (lowpass_t *f)
{
  f->y = 0.0f;
}
```

And a program that uses two filters, without once seeing inside either:

```bash
cat smooth.c
```

```c
/* smooth.c -- two filters through an opaque type.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       smooth.c lowpass.c -o smooth && ./smooth
 *
 * main never sees inside a lowpass_t: it holds two pointers, and every
 * use goes through a function in lowpass.h. Each filter keeps its own
 * memory between calls, so a signal fed in blocks comes out the same as
 * one fed all at once.
 */
#include "lowpass.h"

#include <stdio.h>

enum
{
  N = 8
};

/* A step: silence, then a sudden 1.0, the hardest thing to smooth. */
static void
step (float *buf, size_t n)
{
  for (size_t i = 0; i < n; i++)
    buf[i] = 1.0f;
}

static void
show (const char *label, const float *buf, size_t n)
{
  printf ("%-22s", label);
  for (size_t i = 0; i < n; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
}

int
main (void)
{
  lowpass_t *fast = lowpass_create (0.5f);
  lowpass_t *slow = lowpass_create (0.1f);
  if (fast == NULL || slow == NULL)
    {
      lowpass_destroy (fast); /* both accept NULL: no `if` needed */
      lowpass_destroy (slow);
      return 1;
    }

  float buf[N];
  step (buf, N);
  lowpass_process (fast, buf, N);
  show ("fast (a = 0.5):", buf, N);

  step (buf, N);
  lowpass_process (slow, buf, N);
  show ("slow (a = 0.1):", buf, N);

  /* The same step again, as two blocks of 4, after a reset: */
  lowpass_reset (slow);
  step (buf, N);
  lowpass_process (slow, buf, N / 2);
  lowpass_process (slow, buf + N / 2, N / 2);
  show ("slow, in two blocks:", buf, N);

  lowpass_destroy (fast);
  lowpass_destroy (slow);
  return 0;
}
```

Build both `.c` files into one program, and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined smooth.c lowpass.c -o smooth
./smooth
```

```
fast (a = 0.5):        0.5000 0.7500 0.8750 0.9375 0.9688 0.9844 0.9922 0.9961
slow (a = 0.1):        0.1000 0.1900 0.2710 0.3439 0.4095 0.4686 0.5217 0.5695
slow, in two blocks:   0.1000 0.1900 0.2710 0.3439 0.4095 0.4686 0.5217 0.5695
```

Each filter climbs toward the step's 1.0 at its own pace: the fast one is
halfway after one sample, the slow one a tenth of the way.

## What it shows

- **The header declares, the `.c` defines.** `struct lowpass` has its members
  listed in exactly one place, `lowpass.c`. Everywhere else it is an
  *incomplete type*: usable through a pointer, and in no other way.
- **So it is always on the heap.** A caller can't declare a `lowpass_t`,
  because the compiler doesn't know how many bytes to set aside. The type's own
  code has to make it, and so the API is the create/destroy pair of lesson 10.
  (Exercise 3 shows a way around the heap when that matters.)
- **Each object has its own memory.** `fast` and `slow` each remember their
  last output, `y`, between calls. That is why processing the step in two
  blocks of four gives the same eight samples as processing it in one go: the
  state carries across. Audio arrives in blocks, so this is the property that
  makes a filter usable at all.
- **The wall protects the invariant.** `lowpass_create` refuses an `a` outside
  `0 < a <= 1`, and since nothing outside `lowpass.c` can set `a`, every filter
  that exists has a good one. With the struct in the header, any caller could
  write `f->a = 7.0f` and the filter would blow up. The comparison is written
  `!(a > 0.0f && a <= 1.0f)` so that a NaN, for which every comparison is
  false, is refused too.
- **The implementation is free to change.** Keep `y` in a `double`, add a
  member, reorder them: callers include the same header and compile the same
  way, because they never knew. Exercise 2 links one compiled `smooth.o` with
  two different filters.

## Pitfalls

- **Reaching inside.** [`broken-reach-inside.c`](broken-reach-inside.c) tries
  to set the coefficient directly, and the compiler stops it, because outside
  `lowpass.c` there is no member `a` to set:

  ```
  broken-reach-inside.c:16:4: error: invalid use of incomplete typedef ‘lowpass_t’ {aka ‘struct lowpass’}
  ```

  That error is the point of an opaque type: it is not a wall you are meant to
  climb. If callers need to change `a`, the header gets a `lowpass_set_a` that
  checks the value.

- **Defining the struct in two places.** Copying `struct lowpass { … }` into a
  caller to get at its members compiles, and is a time bomb: the day
  `lowpass.c` changes its struct, the copy is silently wrong, and nothing warns
  you. One definition, in one file.

- **Every call is a function call.** The compiler can't inline
  `lowpass_process` into `smooth.c`, compiled separately (short of link-time
  optimisation). Hide the struct, and give the functions whole blocks to work
  on, as `lowpass_process` does, never one sample at a time: one call per 256
  samples costs nothing.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Declare a `lowpass_t` as a local variable, `lowpass_t f;`, instead of
   creating one, and print `sizeof (lowpass_t)`. What does the compiler say?

   <details><summary>Output and answer</summary>

   [`broken-on-the-stack.c`](exercises/ex1-on-the-stack/broken-on-the-stack.c)
   does both. gcc refuses each:

   ```
   broken-on-the-stack.c:14:13: error: storage size of ‘f’ isn’t known
   broken-on-the-stack.c:15:28: error: invalid application of ‘sizeof’ to incomplete type ‘lowpass_t’ {aka ‘struct lowpass’}
   ```

   To set aside room for `f` on the stack, the compiler must know how many
   bytes a `lowpass_t` takes, and outside `lowpass.c` nobody does. The same
   ignorance is what lets `lowpass.c` change the struct freely: a size the
   caller knew would be a size the caller depends on.

   </details>

2. Keep the filter's coefficient and memory in `double` instead of `float`.
   Which files change? Which have to be compiled again?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-double-state/`:

   Compile all three files once, without linking (`-c`):

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c smooth.c lowpass.c lowpass_double.c
   ```

   Then link the one `smooth.o` with the `float` filter, and run it:

   ```bash
   cc -fsanitize=address,undefined smooth.o lowpass.o -o smooth
   ./smooth
   ```

   ```
   fast (a = 0.5):        0.5000 0.7500 0.8750 0.9375 0.9688 0.9844 0.9922 0.9961
   slow (a = 0.1):        0.1000 0.1900 0.2710 0.3439 0.4095 0.4686 0.5217 0.5695
   slow, in two blocks:   0.1000 0.1900 0.2710 0.3439 0.4095 0.4686 0.5217 0.5695
   ```

   And the same `smooth.o` with the `double` one:

   ```bash
   cc -fsanitize=address,undefined smooth.o lowpass_double.o -o smooth
   ./smooth
   ```

   ```
   fast (a = 0.5):        0.5000 0.7500 0.8750 0.9375 0.9688 0.9844 0.9922 0.9961
   slow (a = 0.1):        0.1000 0.1900 0.2710 0.3439 0.4095 0.4686 0.5217 0.5695
   slow, in two blocks:   0.1000 0.1900 0.2710 0.3439 0.4095 0.4686 0.5217 0.5695
   ```

   Only [`lowpass_double.c`](exercises/ex2-double-state/lowpass_double.c)
   differs from `lowpass.c`, and only it was compiled again: `smooth.o` was
   built once and linked against both. The struct doubled in size, and the
   caller couldn't tell, because it never knew the size. With the struct in the
   header, every file that included it would have to be recompiled, and in a
   library that means every program that uses it. (The outputs agree to four
   decimals; the `double` version rounds less along the way.)

   </details>

3. `lowpass_create` calls `malloc`, which real-time code (lesson 13) does only
   at setup. Change the API so that the caller provides the memory, while the
   struct stays hidden.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-caller-memory/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined bank.c lowpass.c -o bank
   ./bank
   ```

   ```
   16 bytes per filter, 64 in one block
   a = 0.1: 0.1000 0.1900 0.2710 0.3439
   a = 0.2: 0.2000 0.3600 0.4880 0.5904
   a = 0.4: 0.4000 0.6400 0.7840 0.8704
   a = 0.8: 0.8000 0.9600 0.9920 0.9984
   ```

   The caller can't use `sizeof`, so
   [`lowpass.h`](exercises/ex3-caller-memory/lowpass.h) *tells* it the size,
   with a function: `lowpass_size()`. The caller finds that many bytes wherever
   it likes, here one `malloc` at setup for four filters, and `lowpass_init`
   sets a filter up in them. `lowpass_size` rounds up to a multiple of 16, as
   lesson 13's arena did, so each filter in the block starts suitably aligned.
   There is no destroy: the filter owns nothing, and freeing the block frees
   all four. Many real-time libraries offer exactly this pair, `_size` and
   `_init`, beside `_create`.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                | header       | what it does                                          |
| ------------------------------------------------------- | ------------ | ----------------------------------------------------- |
| [`free`](https://en.cppreference.com/c/memory/free)     | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc` |
| [`malloc`](https://en.cppreference.com/c/memory/malloc) | `<stdlib.h>` | allocate memory, contents unset                       |
| [`printf`](https://en.cppreference.com/c/io/fprintf)    | `<stdio.h>`  | formatted output to `stdout`                          |

<!-- /libfuncs -->
