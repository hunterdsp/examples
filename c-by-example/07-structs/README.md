# 07 · Structs

Run this lesson's commands in `c-by-example/07-structs/`.

A **biquad** filter, the workhorse of audio equalisers and lesson 21, needs
five coefficients and two numbers of memory. Seven loose `float`s passed to
every call is seven chances to swap two. A `struct` bundles them, by name.

Here is the program:

```bash
cat structs.c
```

```c
/* structs.c -- grouping values that belong together.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       structs.c -o structs && ./structs
 *
 * A biquad is the workhorse IIR filter of lesson 21: five coefficients
 * and two numbers of memory. Passing seven loose floats to every call
 * invites mistakes; a struct keeps them together, named.
 */
#include <stddef.h> /* offsetof */
#include <stdio.h>

/* The coefficients: what the filter IS. */
struct biquad_coeffs
{
  float b0, b1, b2; /* feed-forward: how much of each input */
  float a1, a2;     /* feedback: how much of each past output */
};

/* The state: what the filter REMEMBERS between samples. */
struct biquad
{
  struct biquad_coeffs c;
  float                z1, z2;
};

/* A typedef gives a struct type a one-word name. */
typedef struct biquad biquad_t;

/* By POINTER: the function updates the caller's state, and no copy of the
   struct is made. This is how every object in Part III is passed. */
static float
biquad_step (biquad_t *f, float x)
{
  /* Transposed direct form II; lesson 21 derives it. */
  float y = f->c.b0 * x + f->z1; /* p->m is (*p).m */
  f->z1   = f->c.b1 * x - f->c.a1 * y + f->z2;
  f->z2   = f->c.b2 * x - f->c.a2 * y;
  return y;
}

/* By VALUE: a copy arrives, so the caller's struct cannot change. */
static float
dc_gain (struct biquad_coeffs c)
{
  return (c.b0 + c.b1 + c.b2) / (1.0f + c.a1 + c.a2);
}

struct padded
{
  char   tag;   /* 1 byte, then 7 bytes of padding so... */
  double value; /* ...this lands on a multiple of 8 */
  char   flag;  /* 1 byte, then 7 more so an ARRAY of these stays aligned */
};

struct packed
{
  double value; /* largest first: no gaps */
  char   tag;
  char   flag;
};

int
main (void)
{
  /* Designated initializers: name each field. Anything not named is 0, so
     z1 and z2 start cleared. This one is a gentle low-pass filter. */
  biquad_t lp = {
    .c = { .b0 = 0.2f, .b1 = 0.4f, .b2 = 0.2f, .a1 = -0.4f, .a2 = 0.2f },
  };
  printf ("DC gain %.3f, state starts at z1=%.1f z2=%.1f\n", dc_gain (lp.c),
          lp.z1, lp.z2);

  printf ("step response:");
  for (int i = 0; i < 8; i++)
    printf (" %.3f", biquad_step (&lp, 1.0f));
  printf ("\n");

  /* Assignment copies the whole struct (unlike an array, lesson 04). */
  biquad_t copy = lp;
  copy.z1       = 0.0f;
  printf ("copy is independent: lp.z1 = %.3f, copy.z1 = %.1f\n", lp.z1,
          copy.z1);

  printf ("sizeof(struct padded) = %zu, sizeof(struct packed) = %zu\n",
          sizeof (struct padded), sizeof (struct packed));
  printf ("offsetof(struct padded, value) = %zu\n",
          offsetof (struct padded, value));
  return 0;
}
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined structs.c -o structs
./structs
```

```
DC gain 1.000, state starts at z1=0.0 z2=0.0
step response: 0.200 0.680 1.032 1.077 1.024 0.994 0.993 0.998
copy is independent: lp.z1 = 0.801, copy.z1 = 0.0
sizeof(struct padded) = 24, sizeof(struct packed) = 16
offsetof(struct padded, value) = 8
```

## What it shows

- **Data and state, kept apart.** `struct biquad_coeffs` is what the filter
  *is*; `struct biquad` adds what it *remembers* (`z1`, `z2`) between samples.
  That split, between configuration and running state, is the shape of every
  object in Part III.
- **Designated initializers** name each field:
  `{ .b0 = 0.2f, .a1 = -0.4f, … }`. Fields you leave out are zero, so the
  filter's memory starts cleared. They read like documentation and survive
  fields being reordered.
- **Pass by pointer to change it, by value to protect it.** `biquad_step` takes
  `biquad_t *` and updates the caller's state through `f->z1` (`p->m` is short
  for `(*p).m`). `dc_gain` takes the coefficients by value: a copy, which it
  cannot harm.
- **Structs assign; arrays don't.** `copy = lp` copies every field, so the copy
  is independent.
- **`typedef`** gives `struct biquad` the one-word name `biquad_t`.

## The step response

Feed a filter a constant 1 and watch it settle: `0.200 0.680 1.032 1.077 …`. It
overshoots past 1, rings, and settles back to 1, its DC gain. The overshoot
comes from the feedback coefficients `a1` and `a2`: the filter feeds its own
past outputs back in. Lesson 21 shows how to choose them, and what happens when
feedback is too strong.

## Padding

`struct padded` holds 10 bytes of data in 24. The CPU wants a `double` at an
address that is a multiple of 8, so the compiler inserts 7 bytes after `tag`
(`offsetof(…, value)` is 8, not 1) and 7 more after `flag`, so that in an array
the *next* struct's `double` is aligned too. Put the largest members first and
the gaps disappear: `struct packed` is 16. When you have a million of them
(lesson 25), that's a third of your memory and cache.

## Pitfalls

- **Passing big structs by value** copies all of them, every call. Pass a
  pointer, and `const` if the function only reads.
- **`==` doesn't work on structs**, and comparing them with `memcmp` compares
  the padding bytes too, whose values are undefined. Compare field by field.
- **Writing a struct to a file with `fwrite`** writes its padding and its byte
  order (lesson 06). Fine for a scratch file read back by the same program;
  wrong for a file format. Write the fields one at a time.
- **Forgetting to initialise.** `biquad_t f;` inside a function holds garbage.
  `biquad_t f = { 0 };` clears every field.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Add a `biquad_reset(biquad_t *f)` that clears the state and leaves the
   coefficients alone. Run the step response twice with a reset between.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-reset/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined reset.c -o reset
   ./reset
   ```

   ```
   first : 0.200 0.680 1.032 1.077 1.024 0.994 0.993 0.998
   second: 0.200 0.680 1.032 1.077 1.024 0.994 0.993 0.998
   identical: yes
   ```

   [`biquad_reset`](exercises/ex1-reset/reset.c) sets `f->z1` and `f->z2` to 0
   and nothing else. Because the coefficients and the state live in separate
   parts of the struct, "forget the past, keep the filter" is two lines. Every
   stateful object in Part III gets a reset like this.

   </details>

2. Set `.a1 = -1.8f, .a2 = 0.9f` and print 40 steps. Then try `.a2 = 1.1f`.
   What happened?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-feedback/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined feedback.c -o feedback
   ./feedback
   ```

   ```
   a1 = -1.8, a2 = 0.9:
        0.20     0.96     2.35     4.16     6.18     8.18     9.96    11.36
       12.29    12.70    12.60    12.04    11.14    10.02     8.80     7.63
        6.61     5.83     5.35     5.18     5.31     5.69     6.27     6.96
        7.69     8.38     8.96     9.38     9.63     9.68     9.57     9.31
        8.94     8.52     8.08     7.69     7.36     7.13     7.01     7.00
   a1 = -1.8, a2 = 1.1:
        0.20     0.96     2.31     3.90     5.28     6.01     5.82     4.66
        2.78     0.69    -1.02    -1.80    -1.31     0.42     2.99     5.73
        7.82     8.58     7.63     5.10     1.59    -1.95    -4.46    -5.08
       -3.44     0.19     4.93     9.47    12.42    12.74    10.06     4.91
       -1.44    -7.19   -10.55   -10.29    -6.12     1.11     9.53    16.73
   ```

   With `a2 = 0.9` the filter rings: it overshoots to 12.7, swings back down to
   about 5, and slowly settles toward 8 (its DC gain, 0.8 / 0.1). With
   `a2 = 1.1` each swing is bigger than the last: the filter is *unstable*. Its
   own past outputs, fed back, grow faster than they die away, so the output
   grows without limit, and in a real system it would soon hit the largest
   number a `float` holds. Lesson 21 shows how to tell, from the coefficients
   alone, which filters are safe.

   </details>

3. Print `sizeof` of a struct holding `char, int, char, int`. Reorder it to
   shrink it, and check with `offsetof`.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-padding/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined padding.c -o padding
   ./padding
   ```

   ```
   interleaved: sizeof 16, offsets a 0 b 4 c 8 d 12
   sorted:      sizeof 12, offsets b 0 d 4 a 8 c 9
   ```

   Each `int` must start at a multiple of 4, so after each `char` the compiler
   pads 3 bytes: 10 bytes of data in 16. With both `int`s first, the two
   `char`s share one 4-byte slot at the end: 12 bytes, a quarter less. Largest
   members first is the rule of thumb.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                               |
| ------------------------------------------------------------ | ------------ | ---------------------------------------------------------- |
| [`memcmp`](https://en.cppreference.com/c/string/byte/memcmp) | `<string.h>` | compare two blocks of bytes                                |
| [`offsetof`](https://en.cppreference.com/c/types/offsetof)   | `<stddef.h>` | a member's distance from the start of its struct (a macro) |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                               |

<!-- /libfuncs -->
