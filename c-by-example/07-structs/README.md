# 07 · Structs

```bash
cd c-by-example/07-structs
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

A **biquad** filter, the workhorse of audio equalisers and lesson 21, needs
five coefficients and two numbers of memory. Seven loose `float`s passed to
every call is seven chances to swap two. A `struct` bundles them, by name.

## What it shows

- **Data and state, kept apart.** `struct biquad_coeffs` is what the filter
  *is*; `struct biquad` adds what it *remembers* (`z1`, `z2`) between
  samples. That split, between configuration and running state, is the
  shape of every object in Part III.
- **Designated initializers** name each field:
  `{ .b0 = 0.2f, .a1 = -0.4f, … }`. Fields you leave out are zero, so the
  filter's memory starts cleared. They read like documentation and survive
  fields being reordered.
- **Pass by pointer to change it, by value to protect it.** `biquad_step`
  takes `biquad_t *` and updates the caller's state through `f->z1`
  (`p->m` is short for `(*p).m`). `dc_gain` takes the coefficients by value:
  a copy, which it cannot harm.
- **Structs assign; arrays don't.** `copy = lp` copies every field, so the
  copy is independent.
- **`typedef`** gives `struct biquad` the one-word name `biquad_t`.

## The step response

Feed a filter a constant 1 and watch it settle: `0.200 0.680 1.032 1.077 …`.
It overshoots past 1, rings, and settles back to 1, its DC gain. The
overshoot comes from the feedback coefficients `a1` and `a2`: the filter
feeds its own past outputs back in. Lesson 21 shows how to choose them, and
what happens when feedback is too strong.

## Padding

`struct padded` holds 10 bytes of data in 24. The CPU wants a `double` at
an address that is a multiple of 8, so the compiler inserts 7 bytes after
`tag` (`offsetof(…, value)` is 8, not 1) and 7 more after `flag`, so that
in an array the *next* struct's `double` is aligned too. Put the largest
members first and the gaps disappear: `struct packed` is 16. When you have a
million of them (lesson 25), that's a third of your memory and cache.

## Pitfalls

- **Passing big structs by value** copies all of them, every call. Pass a
  pointer, and `const` if the function only reads.
- **`==` doesn't work on structs**, and comparing them with `memcmp`
  compares the padding bytes too, whose values are undefined. Compare field
  by field.
- **Writing a struct to a file with `fwrite`** writes its padding and its
  byte order (lesson 06). Fine for a scratch file read back by the same
  program; wrong for a file format. Write the fields one at a time.
- **Forgetting to initialise.** `biquad_t f;` inside a function holds
  garbage. `biquad_t f = { 0 };` clears every field.

## Exercises

1. Add a `biquad_reset(biquad_t *f)` that clears the state and leaves the
   coefficients alone. Run the step response twice with a reset between.
2. Set `.a1 = -1.8f, .a2 = 0.9f` and print 40 steps. Then try `.a2 = 1.1f`.
   What happened?
3. Print `sizeof` of a struct holding `char, int, char, int`. Reorder it to
   shrink it, and check with `offsetof`.
