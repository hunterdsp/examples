# 19 · Measuring first

Run this lesson's commands in `c-by-example/19-measuring/`.

Part IV is about speed: filters, oscillators and transforms that keep up with
real time, and how the machine underneath decides what "fast" means. It starts
with the rule that every lesson after it follows:

> **Measure before you change anything, and after.**

Intuition about speed is wrong surprisingly often. The compiler has already
made some of the improvements you would make, the machine is fast at things
that look slow and slow at things that look fast, and the part of a program
that takes the time is rarely the part you were looking at. A measurement
settles it, and takes a minute.

Here are two ways to make a sine wave: calling `sin()` for every sample, or
looking it up in a table of 1024 values made once, with straight-line
interpolation between neighbours. The program checks that they agree, and times
them when asked:

```bash
cat bench.c
```

```c
/* bench.c -- two ways to make a sine wave, checked, then timed.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       bench.c -o bench -lm && ./bench
 *   cc -std=c99 -Wall -Wextra -O2 bench.c -o bench -lm && ./bench time
 *
 * One way calls sin() for every sample. The other looks the sine up in a
 * table of 1024 values, made once, and interpolates between neighbours
 * (lesson 03's table, put to work). Run plainly, the program checks that
 * the two agree. Run with `time`, it also measures each one, which is
 * only meaningful in an optimised build without sanitizers.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N     = 4800, /* 0.1 s at 48 kHz */
  TABLE = 1024, /* a power of two: wrap with a mask (lesson 12) */
  RUNS  = 7,    /* time this many times, keep the fastest */
  REPS  = 200   /* calls per timing: long enough for clock() to see */
};

static const double TWO_PI = 6.283185307179586;
static const double FREQ   = 440.0 / 48000.0; /* cycles per sample */

static float table[TABLE + 1]; /* one extra, a copy of [0], for [i + 1] */
static float out[N];

static void
by_sin (float *y, size_t n)
{
  for (size_t i = 0; i < n; i++)
    y[i] = (float)sin (TWO_PI * FREQ * (double)i);
}

static void
by_table (float *y, size_t n)
{
  for (size_t i = 0; i < n; i++)
    {
      double pos  = FREQ * (double)i * (double)TABLE; /* in table steps */
      double base = floor (pos);
      size_t k    = (size_t)base & (size_t)(TABLE - 1);
      float  frac = (float)(pos - base);
      y[i]        = table[k] + frac * (table[k + 1] - table[k]);
    }
}

typedef void (*gen_fn) (float *y, size_t n);

/* CPU seconds for REPS calls of f, the fastest of RUNS tries. The first
   tries also warm up the caches; the minimum is the least disturbed by
   whatever else the machine was doing. */
static double
best_seconds (gen_fn f)
{
  double best = 1e30;
  for (int run = 0; run < RUNS; run++)
    {
      clock_t start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        f (out, N);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      if (s < best)
        best = s;
    }
  return best;
}

int
main (int argc, char **argv)
{
  for (size_t k = 0; k <= TABLE; k++)
    table[k] = (float)sin (TWO_PI * (double)k / (double)TABLE);

  /* First: are they the same wave? A fast wrong answer is no use. */
  static float a[N], b[N];
  by_sin (a, N);
  by_table (b, N);
  double worst = 0.0;
  for (size_t i = 0; i < N; i++)
    {
      double d = fabs ((double)a[i] - (double)b[i]);
      if (d > worst)
        worst = d;
    }
  printf ("%d samples, largest difference below 1e-5: %s\n", N,
          worst < 1e-5 ? "yes" : "no");

  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      double t_sin   = best_seconds (by_sin);
      double t_table = best_seconds (by_table);
      double per     = 1e9 / ((double)REPS * (double)N);
      printf ("sin():  %5.2f ns per sample\n", t_sin * per);
      printf ("table:  %5.2f ns per sample\n", t_table * per);
      printf ("table is %.1f times as fast\n", t_sin / t_table);
      /* Use the output, so the compiler can't decide it was never needed
         and skip making it (exercise 3). */
      printf ("(last sample %.4f)\n", (double)out[N - 1]);
    }
  return 0;
}
```

Build it the course's way, and check the two waves agree:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined bench.c -o bench -lm
./bench
```

```
4800 samples, largest difference below 1e-5: yes
```

To time it, build it the way it would ship: optimised (`-O2`), without the
sanitizers, which slow everything down, and not evenly:

```bash
cc -std=c99 -Wall -Wextra -O2 bench.c -o bench -lm
```

<!-- no-run -->

```bash
./bench time
```

On an x86-64 laptop (AMD Ryzen, Linux), with gcc 15:

```
4800 samples, largest difference below 1e-5: yes
sin():   8.42 ns per sample
table:   1.72 ns per sample
table is 4.9 times as fast
(last sample -0.0576)
```

Your numbers will differ, from machine to machine and from run to run; the
ratio is what to look at. (This test can't check them, so `make test` runs only
the plain `./bench`.)

## What it shows

- **Correct first.** The program compares the two waves before it times
  anything. The table is less exact than `sin()`, by under 0.00001 here, and
  whether that is good enough is a decision for the sound, not the stopwatch.
- **Time the build you ship.** `-O2` without sanitizers. A debug build measures
  the debugger's convenience; a sanitized build measures the sanitizer's
  checks, and changes the *ratio*, not only the totals (exercise 1).
- **`clock()` measures CPU time**, the processor time this program used, in
  units of `CLOCKS_PER_SEC`. It ignores time spent waiting, which is what you
  want when comparing two loops. Wall-clock time, what a user waits through,
  comes from a timer outside standard C, such as POSIX `clock_gettime`.
- **Repeat, and keep the fastest.** One call of `by_sin` takes about 40
  microseconds, too short for `clock()` to see reliably, so each timing runs it
  200 times. And each timing is taken 7 times, keeping the fastest: the first
  runs warm the caches, and the fastest run is the one least disturbed by
  whatever else the machine was doing.
- **Divide it down.** "8.42 ns per sample" compares directly with the budget:
  at 48 kHz a sample is due every 20,800 ns.
- **Use the result.** The program prints a sample of the output after timing. A
  result nobody uses is one the compiler may decide not to compute (exercise
  3).

## Pitfalls

- **Benchmarking the debug build.** At `-O0` the comparison can come out
  backwards: exercise 2's "obvious" improvement is 3.5 times as fast there, and
  makes no difference at all at `-O2`.
- **Timing something too short.** `clock()` counts in steps that may be as
  coarse as a millisecond. Repeat the work until a timing takes tens of
  milliseconds.
- **A benchmark unlike the real program.** These loops run on 4800 samples that
  fit in the cache (lesson 25), over and over. A real program's data may not,
  and a lookup table competes with everything else for that cache.
- **Optimising what doesn't matter.** A program that spends 1% of its time
  making sine waves gains at most 1% from a faster one. Measure the whole
  program to find where the time goes, before measuring the part you suspect;
  `perf` on Linux and Instruments on macOS do that for you.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Time `bench.c` built the course's usual way, with the sanitizers. Does the
   table still win, and by as much?

   <details><summary>Output and answer</summary>

   This one uses the lesson's own `bench.c`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -O1 -fsanitize=address,undefined bench.c -o bench-asan -lm
   ```

   <!-- no-run -->

   ```bash
   ./bench-asan time
   ```

   On the same machine:

   ```
   4800 samples, largest difference below 1e-5: yes
   sin():   8.16 ns per sample
   table:   4.77 ns per sample
   table is 1.7 times as fast
   (last sample -0.0576)
   ```

   `sin()` hardly changed: its time is spent inside the math library, which
   isn't instrumented. The table loop is almost three times slower, because
   every array access now comes with a check that it is in bounds. So the
   sanitized build says the table is 1.7 times as fast, and the shipped build
   says 4.9. Sanitizers are for finding bugs; for timing, turn them off.

   </details>

2. Squaring with `pow(x, 2.0)` looks wasteful next to `x * x`. Measure both, at
   `-O0` and at `-O2`.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-pow/`:

   ```bash
   cc -std=c99 -Wall -Wextra -O0 pow.c -o pow -lm
   ./pow
   ```

   ```
   same energy: yes
   ```

   <!-- no-run -->

   ```bash
   ./pow time
   ```

   ```
   same energy: yes
   pow:    21.68 ns per sample
   x * x:   6.14 ns per sample
   ```

   And again at `-O2`:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 pow.c -o pow -lm
   ```

   <!-- no-run -->

   ```bash
   ./pow time
   ```

   ```
   same energy: yes
   pow:     0.52 ns per sample
   x * x:   0.52 ns per sample
   ```

   At `-O0`, `pow` is 3.5 times slower. At `-O2` there is no difference: the
   compiler knows that `pow(x, 2.0)` is exactly `x * x`, and writes that for
   you. Rewriting it by hand would have been a change made for nothing,
   justified by measuring the wrong build. (Write `x * x` anyway if it reads
   better; just don't expect it to be faster.)

   </details>

3. [`dead.c`](exercises/ex3-dead-code/dead.c) times the same loop twice: once
   throwing its result away, once keeping it. What does `-O2` make of the
   first?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-dead-code/`:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 dead.c -o dead
   ./dead
   ```

   ```
   sum of 4800 samples of 0.001: 4.800
   ```

   <!-- no-run -->

   ```bash
   ./dead time
   ```

   ```
   sum of 4800 samples of 0.001: 4.800
   result unused:  0.00 ns per sample
   result kept:    0.49 ns per sample
   ```

   Zero: the compiler saw that nothing uses the unused sums, and removed the
   loop. That is correct, and exactly what an optimiser should do, but it makes
   a benchmark measure nothing and report an impossible speed. Storing the
   result in a `volatile` variable tells the compiler that the store itself
   matters, so the work must be done. A result that seems too good to be true
   usually is: check that the work happened.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                                  |
| ------------------------------------------------------------ | ------------ | ------------------------------------------------------------- |
| [`clock`](https://en.cppreference.com/c/chrono/clock)        | `<time.h>`   | processor time used by the program, in `CLOCKS_PER_SEC` units |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)    | `<math.h>`   | absolute value of a `double`                                  |
| [`floor`](https://en.cppreference.com/c/numeric/math/floor)  | `<math.h>`   | largest whole number not above x                              |
| [`pow`](https://en.cppreference.com/c/numeric/math/pow)      | `<math.h>`   | x raised to the power y                                       |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                                  |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)      | `<math.h>`   | sine of an angle in radians                                   |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp) | `<string.h>` | compare two strings                                           |

<!-- /libfuncs -->
