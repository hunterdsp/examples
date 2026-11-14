# 26 · Vectorisation

Run this lesson's commands in `c-by-example/26-vectorisation/`.

Every processor this course runs on has **vector** instructions, which do the
same operation on several numbers at once: SSE and AVX on x86, NEON on ARM. One
instruction adds 4 pairs of floats, or 8, or 16. A loop that uses them can run
several times faster, and in signal processing, where the same arithmetic is
done to every sample, that is most loops.

You rarely write those instructions yourself. At `-O2` the compiler
**vectorises** a loop on its own, when it can prove that doing 4 or 8
iterations at once gives exactly the result the loop as written would give. The
job is mostly to write loops it can prove that about, and the most important
loop it can't is the most common one in DSP: a sum.

```bash
cat vec.c
```

```c
/* vec.c -- one sum, added up in two orders.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       vec.c -o vec && ./vec
 *   cc -std=c99 -Wall -Wextra -O2 vec.c -o vec && ./vec time
 *
 * A processor's VECTOR instructions (SSE and AVX on x86, NEON on ARM)
 * work on several floats at once: 4, 8 or 16 in one instruction. The
 * compiler uses them for a loop when it can prove the result is the
 * same. A float sum is the classic loop it can NOT: adding in a
 * different order rounds differently, and so gives a different answer.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N     = 1 << 20, /* 1048576 samples */
  LANES = 8,
  REPS  = 100
};

static float x[N];

/* One running total: each addition waits for the one before. */
static float
sum_in_order (const float *v, size_t n)
{
  float s = 0.0f;
  for (size_t i = 0; i < n; i++)
    s += v[i];
  return s;
}

/* Eight running totals, side by side, combined at the end. A DIFFERENT
   order of additions, written out, so the compiler is free to put the
   eight lanes in one vector register and add eight at a time. */
static float
sum_in_lanes (const float *v, size_t n)
{
  float  lane[LANES] = { 0.0f };
  size_t i           = 0;
  for (; i + LANES <= n; i += LANES)
    for (size_t k = 0; k < LANES; k++)
      lane[k] += v[i + k];
  for (; i < n; i++) /* the last n % 8, if any */
    lane[0] += v[i];
  for (size_t width = LANES / 2; width > 0; width /= 2) /* 8 -> 4 -> 2 -> 1 */
    for (size_t k = 0; k < width; k++)
      lane[k] += lane[k + width];
  return lane[0];
}

typedef float (*sum_fn) (const float *v, size_t n);

static volatile float sink;

static double
best_ms (sum_fn f)
{
  double best = 1e30;
  for (int run = 0; run < 5; run++)
    {
      clock_t start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        sink = f (x, N);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      best     = s < best ? s : best;
    }
  return best * 1e3 / (double)REPS;
}

int
main (int argc, char **argv)
{
  double exact = 0.0;
  for (size_t i = 0; i < N; i++)
    {
      x[i] = 0.1f;
      exact += (double)x[i]; /* double: as good as exact here */
    }
  printf ("%d samples of 0.1f\n", N);
  printf ("in order:  %.4f\n", (double)sum_in_order (x, N));
  printf ("in lanes:  %.4f\n", (double)sum_in_lanes (x, N));
  printf ("in double: %.4f\n", exact);

  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      printf ("in order: %6.3f ms\n", best_ms (sum_in_order));
      printf ("in lanes: %6.3f ms\n", best_ms (sum_in_lanes));
    }
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined vec.c -o vec
./vec
```

```
1048576 samples of 0.1f
in order:  105891.8438
in lanes:  104748.9531
in double: 104857.6016
```

Timed:

```bash
cc -std=c99 -Wall -Wextra -O2 vec.c -o vec
```

<!-- no-run -->

```bash
./vec time
```

On the machine from lesson 19, with gcc 15 (clang 21 is within 15%):

```
1048576 samples of 0.1f
in order:  105891.8438
in lanes:  104748.9531
in double: 104857.6016
in order:  0.549 ms
in lanes:  0.068 ms
```

## What it shows

- **Order changes a float sum.** A million copies of `0.1f` add up to 104,857.6
  (the `double` total, which is as good as exact here). Added in order into one
  `float`, the answer is 1% high; in eight lanes, 0.1% low. Each addition
  rounds to 24 bits, and how much it loses depends on how big the running total
  already is: one total of 100,000 loses more on each `+ 0.1` than eight totals
  of 13,000.
- **So the compiler keeps your order.** Vectorising `sum_in_order` would mean
  adding in lanes, which changes the answer, so the compiler doesn't, and every
  addition waits for the one before it.
- **Write the order you want.** `sum_in_lanes` *says* it adds in eight lanes.
  That is now the order as written, so the compiler can put the lanes in vector
  registers and add eight at a time: 8 times faster. You chose the rounding,
  explicitly, instead of the compiler choosing it for you.
- **Side by side, every iteration independent.** Loops that vectorise read and
  write consecutive elements (lesson 25's struct of arrays) and don't depend on
  the previous iteration's result. The eight lanes are independent of each
  other; one running total is not.
- **The remainder.** `n` needn't be a multiple of 8, so a short loop finishes
  the last `n % 8` samples one at a time.

## Pitfalls

- **Running off the end in lanes.** [`broken-lane-tail.c`](broken-lane-tail.c)
  steps by 8 with `i < n` instead of `i + 8 <= n`. With 1003 samples, the last
  step reads 5 floats past the end:

  ```
  broken-lane-tail.c:29:19: runtime error: load of address 0x7d1aad3e10ac with insufficient space for an object of type 'float'
  ```

- **`-ffast-math`.** This flag lets the compiler reorder floating-point
  arithmetic as if it were exact, which vectorises sums, and changes their
  answers in ways that differ from compiler to compiler (exercise 2). It also
  assumes there are no NaNs or infinities, so checks for them may be removed.
  Use it knowingly, on code where you have measured both speed and accuracy.

- **The baseline instruction set.** By default, compilers for x86-64 use only
  the vector instructions every x86-64 processor has (SSE2: 4 floats at a
  time). `-march=native` allows everything the build machine has, AVX2 and
  more, and produces a program that may crash with an illegal instruction on an
  older machine. For code you ship, choose the target on purpose.

- **Vectorised doesn't mean fast.** A vectorised loop over data that isn't in
  the cache still waits on memory (lesson 25). Measure.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Compilers can report what they vectorised. Ask gcc (`-fopt-info-vec`) and
   clang (`-Rpass=loop-vectorize`) about
   [`gain.c`](exercises/ex1-report/gain.c), whose two functions differ only in
   `restrict`.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-report/`:

   The plain build:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 -c gain.c
   ```

   gcc 15, at `-O2` and then `-O3`. The flags are gcc's, and the output differs
   from version to version:

   <!-- no-run -->

   ```bash
   gcc -std=c99 -Wall -Wextra -O2 -fopt-info-vec -c gain.c
   gcc -std=c99 -Wall -Wextra -O3 -fopt-info-vec -c gain.c
   ```

   ```
   gain.c:24:24: optimized: loop vectorized using 16 byte vectors
   gain.c:14:24: optimized: loop vectorized using 16 byte vectors
   gain.c:14:24: optimized:  loop versioned for vectorization because of possible aliasing
   gain.c:14:24: optimized: loop vectorized using 8 byte vectors
   gain.c:24:24: optimized: loop vectorized using 16 byte vectors
   gain.c:24:24: optimized: loop vectorized using 8 byte vectors
   ```

   And clang 21:

   <!-- no-run -->

   ```bash
   clang -std=c99 -Wall -Wextra -O2 -Rpass=loop-vectorize -c gain.c
   ```

   ```
   gain.c:14:3: remark: vectorized loop (vectorization width: 4, interleaved count: 2) [-Rpass=loop-vectorize]
   gain.c:24:3: remark: vectorized loop (vectorization width: 4, interleaved count: 2) [-Rpass=loop-vectorize]
   ```

   At `-O2`, gcc vectorises only `gain_restrict` (line 24). For `gain`, `out`
   might overlap `in`, and writing eight outputs at once could overwrite inputs
   not yet read. At `-O3` gcc vectorises it too, by *versioning*: a check at
   run time whether the two overlap, with the vector loop for when they don't
   and the plain loop for when they do. clang does that already at `-O2`.
   `restrict` (lesson 18) is the caller's promise that makes the check
   unnecessary.

   </details>

2. Build the lesson's `vec.c` with `-O2 -ffast-math`, with gcc and with clang.
   What happens to the sums?

   <details><summary>Output and answer</summary>

   This one uses the lesson's own `vec.c`.

   <!-- no-run -->

   ```bash
   gcc -std=c99 -O2 -ffast-math vec.c -o vec
   ./vec time
   ```

   ```
   1048576 samples of 0.1f
   in order:  104598.4766
   in lanes:  104748.9531
   in double: 104857.6016
   in order:  0.555 ms
   in lanes:  0.070 ms
   ```

   <!-- no-run -->

   ```bash
   clang -std=c99 -O2 -ffast-math vec.c -o vec
   ./vec time
   ```

   ```
   1048576 samples of 0.1f
   in order:  104748.9531
   in lanes:  104841.4141
   in double: 104857.6016
   in order:  0.070 ms
   in lanes:  0.147 ms
   ```

   Every float sum changed, and differently with each compiler. clang
   vectorised `sum_in_order`, so it is now as fast as the lanes were, and gives
   the lanes' old answer; it also reorganised `sum_in_lanes`, which now gives a
   third answer, more accurately and more slowly. gcc reordered `sum_in_order`
   without speeding it up. With `-ffast-math`, the source no longer decides the
   order of the arithmetic, so it no longer decides the answer. Only the
   `double` total stayed put.

   </details>

3. Accumulate the eight lanes in `double` instead of `float`. How much more
   accurate is it, and what does it cost?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-double/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sums.c -o sums
   ./sums
   ```

   ```
   float lanes:  104748.9453
   double lanes: 104857.6016
   ```

   Timed:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 sums.c -o sums
   ```

   <!-- no-run -->

   ```bash
   ./sums time
   ```

   With gcc 15:

   ```
   float lanes:  104748.9453
   double lanes: 104857.6016
   float lanes:   0.069 ms
   double lanes:  0.367 ms
   ```

   The `double` lanes get the exact total; the `float` lanes are off by 0.1%.
   It cost 5 times the time with gcc and twice with clang (0.130 ms): a 16-byte
   SSE register holds 4 floats but only 2 doubles, and converting each float
   adds work. For audio, a `float` sum is usually accurate enough; for long
   sums, statistics, or anything where 0.1% matters, accumulate in `double` and
   keep the samples in `float`. Lesson 02 said the same thing, before there was
   a measurement to back it.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                                  |
| ------------------------------------------------------------ | ------------ | ------------------------------------------------------------- |
| [`clock`](https://en.cppreference.com/c/chrono/clock)        | `<time.h>`   | processor time used by the program, in `CLOCKS_PER_SEC` units |
| [`free`](https://en.cppreference.com/c/memory/free)          | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc`         |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)      | `<stdlib.h>` | allocate memory, contents unset                               |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                                  |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp) | `<string.h>` | compare two strings                                           |

<!-- /libfuncs -->
