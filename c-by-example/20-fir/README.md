# 20 · FIR filters

Run this lesson's commands in `c-by-example/20-fir/`.

A **filter** changes how much of each frequency gets through. A low-pass filter
lets the low ones pass and holds back the high ones: it takes the hiss off a
recording, or keeps a signal below half the sample rate before it is resampled.

The simplest filter to build is the **finite impulse response** (FIR) filter.
Each output is a weighted sum of the latest input and the ones just before it:

```
y[i] = h[0]·x[i] + h[1]·x[i-1] + … + h[n-1]·x[i-(n-1)]
```

The weights `h` are the filter's **taps**. Lesson 04's moving average was one,
with every tap equal (exercise 1). Choose the taps well and the same loop is a
low-pass, a high-pass, or anything between.

The filter, as an opaque type (lesson 14) with lesson 18's API shape:

```bash
cat fir.h
```

```c
/* fir.h -- a finite impulse response (FIR) filter.
 *
 * Each output is a weighted sum of the last n inputs:
 *
 *   y[i] = h[0] x[i] + h[1] x[i-1] + ... + h[n-1] x[i-(n-1)]
 *
 * The weights h are the filter's TAPS; choosing them chooses what the
 * filter does. The filter keeps the last n inputs between calls, so a
 * signal may be processed in blocks of any size.
 */
#ifndef FIR_H
#define FIR_H

#include <stddef.h>

typedef struct fir fir_t;

/* A filter with a copy of the n taps (n > 0). The caller owns the
   result; free it with fir_destroy. NULL if memory is short. */
fir_t *fir_create (const float *taps, size_t n);

/* Free f. Accepts NULL. */
void fir_destroy (fir_t *f);

/* Filter n samples from in into out. in and out may be the same buffer. */
void fir_process (fir_t *f, const float *in, float *out, size_t n);

/* Forget past inputs: the next output is as if the input before it were
   all zeros. */
void fir_reset (fir_t *f);

#endif /* FIR_H */
```

`fir.c` keeps the taps and the last `n` inputs in one allocation, using a
feature of C99 this course hasn't needed until now:

```bash
cat fir.c
```

```c
/* fir.c -- the FIR filter, in one allocation. */
#include "fir.h"

#include <stdlib.h>
#include <string.h>

struct fir
{
  size_t n;      /* number of taps */
  size_t newest; /* where in hist the latest input is */
  float *hist;   /* the last n inputs: a ring (lesson 12) */
  float  taps[]; /* n taps, then the n history slots: see fir_create */
};

fir_t *
fir_create (const float *taps, size_t n)
{
  /* taps[] is a FLEXIBLE ARRAY MEMBER (C99): the struct ends in an array
     of no fixed size, and one malloc makes room for the struct plus as
     many floats as asked for, here 2n: n taps and n history slots. */
  fir_t *f = malloc (sizeof *f + (size_t)2 * n * sizeof f->taps[0]);
  if (f == NULL)
    return NULL;
  f->n    = n;
  f->hist = f->taps + n;
  memcpy (f->taps, taps, n * sizeof taps[0]);
  fir_reset (f);
  return f;
}

void
fir_destroy (fir_t *f)
{
  free (f); /* one malloc, so one free */
}

void
fir_reset (fir_t *f)
{
  for (size_t k = 0; k < f->n; k++)
    f->hist[k] = 0.0f;
  f->newest = 0;
}

void
fir_process (fir_t *f, const float *in, float *out, size_t n)
{
  for (size_t i = 0; i < n; i++)
    {
      /* Move on one slot, overwriting the oldest input with the newest. */
      f->newest          = f->newest + 1 < f->n ? f->newest + 1 : 0;
      f->hist[f->newest] = in[i];

      /* Tap k meets the input from k samples ago, k slots back. */
      float  acc = 0.0f;
      size_t j   = f->newest;
      for (size_t k = 0; k < f->n; k++)
        {
          acc += f->taps[k] * f->hist[j];
          j = j > 0 ? j - 1 : f->n - 1;
        }
      out[i] = acc;
    }
}
```

A program that designs a 31-tap low-pass with its corner at 4 kHz, then
measures it:

```bash
cat lowpass.c
```

```c
/* lowpass.c -- design a low-pass FIR filter, then measure what it does.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       lowpass.c fir.c -o lowpass -lm && ./lowpass
 *
 * The taps are a WINDOWED SINC: the ideal low-pass filter's impulse
 * response (a sinc), cut to 31 taps and tapered at the ends by a Hamming
 * window so the cut doesn't ring. The program then checks the filter the
 * way an engineer would: feed it an impulse, a step, and sine waves, and
 * look at what comes out.
 */
#include "fir.h"

#include <math.h>
#include <stdio.h>

enum
{
  TAPS = 31,
  LEN  = 4800 /* 0.1 s at 48 kHz */
};

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

/* Windowed-sinc low-pass taps, passing frequencies below cutoff_hz. */
static void
design (float *h, size_t n, double cutoff_hz)
{
  double fc  = cutoff_hz / RATE; /* cycles per sample */
  double mid = (double)(n - 1) / 2.0;
  double sum = 0.0;
  for (size_t k = 0; k < n; k++)
    {
      double m    = (double)k - mid;
      double sinc = m == 0.0 ? 2.0 * fc : sin (2.0 * PI * fc * m) / (PI * m);
      double win  = 0.54 - 0.46 * cos (2.0 * PI * (double)k / (double)(n - 1));
      h[k]        = (float)(sinc * win);
      sum += sinc * win;
    }
  for (size_t k = 0; k < n; k++)
    h[k] = (float)((double)h[k] / sum); /* gain 1 at 0 Hz */
}

/* The filter's gain at freq_hz, in dB: run a sine through it and compare
   the output's peak, once the first TAPS samples have passed, with 1. */
static double
gain_db (fir_t *f, double freq_hz)
{
  static float x[LEN];
  for (size_t i = 0; i < LEN; i++)
    x[i] = (float)sin (2.0 * PI * freq_hz * (double)i / RATE);
  fir_reset (f);
  fir_process (f, x, x, LEN); /* in place */
  float peak = 0.0f;
  for (size_t i = TAPS; i < LEN; i++)
    peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
  return 20.0 * log10 ((double)peak);
}

int
main (void)
{
  float h[TAPS];
  design (h, TAPS, 4000.0);
  fir_t *f = fir_create (h, TAPS);
  if (f == NULL)
    return 1;

  /* An impulse in: the taps come out, one per sample. */
  float x[TAPS] = { 1.0f }, y[TAPS];
  fir_process (f, x, y, TAPS);
  int same = 1;
  for (size_t k = 0; k < TAPS; k++)
    same = same && y[k] == h[k];
  printf ("impulse response is the taps: %s\n", same ? "yes" : "no");

  /* A step in: the output settles at the sum of the taps, the DC gain. */
  float step[2 * TAPS];
  for (size_t i = 0; i < 2 * TAPS; i++)
    step[i] = 1.0f;
  fir_reset (f);
  fir_process (f, step, step, 2 * TAPS);
  printf ("step settles at %.4f\n", (double)step[2 * TAPS - 1]);

  /* Sines in: how much of each frequency gets through. */
  const double freqs[] = { 1000.0, 3000.0, 4000.0, 6000.0, 12000.0 };
  for (size_t i = 0; i < sizeof freqs / sizeof freqs[0]; i++)
    printf ("%6.0f Hz: %7.2f dB\n", freqs[i], gain_db (f, freqs[i]));

  fir_destroy (f);
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined lowpass.c fir.c -o lowpass -lm
./lowpass
```

```
impulse response is the taps: yes
step settles at 1.0000
  1000 Hz:   -0.01 dB
  3000 Hz:   -1.90 dB
  4000 Hz:   -6.03 dB
  6000 Hz:  -29.54 dB
 12000 Hz:  -56.30 dB
```

Below the 4 kHz corner the signal passes almost untouched; at the corner it is
halved (−6 dB, lesson 02); an octave above, at 12 kHz, it is down to a
seven-hundredth of its size.

## What it shows

- **The impulse response *is* the taps.** Feed in a single 1 followed by zeros,
  and out come `h[0]`, `h[1]`, … in turn. That is where the name comes from:
  the response to an impulse is finite, `n` samples long, and then exactly
  zero. It is also the first test any FIR filter should pass.
- **The step response settles at the sum of the taps**, the filter's gain at 0
  Hz. `design` divides by that sum, so a constant comes through unchanged.
- **Measure the frequency response by playing tones.** A sine through a filter
  comes out as a sine of the same frequency, scaled; the scale is the gain.
  `gain_db` skips the first `TAPS` samples, while the history still holds the
  silence before the tone.
- **A windowed sinc.** The ideal low-pass filter's impulse response is the
  *sinc* function, `sin(x)/x`, which never ends. `design` keeps the middle 31
  values and tapers them toward the ends with a Hamming window; cutting them
  off abruptly instead would let through ripples of the frequencies it should
  block.
- **A flexible array member.** `float taps[];` as the last member of a struct
  has no size of its own: `malloc (sizeof *f + 2 * n * sizeof f->taps[0])`
  makes room for the struct and `2n` floats after it, all in one block. One
  allocation, one `free`, and the samples sit right next to the struct that
  uses them.
- **The cost is `n` multiply-adds per sample.** 31 taps at 48 kHz is 1.5
  million a second, which is nothing; but a sharper filter needs more taps
  (exercise 3), and a smarter inner loop starts to matter (exercise 2).

## Pitfalls

- **A history one sample short.**
  [`broken-short-history.c`](broken-short-history.c) keeps the `n - 1`
  *previous* inputs and forgets that the current one needs a slot too. The
  first time the ring reaches its last slot, it is past the end of the block,
  and UBSan, which knows how big the block is, stops it:

  ```
  broken-short-history.c:34:32: runtime error: load of address 0x771cf4de001c with insufficient space for an object of type 'float'
  ```

  Built without UBSan's check, AddressSanitizer catches the same read, as a
  `heap-buffer-overflow`.

- **Taps in the wrong order.** `h[0]` multiplies the *newest* input. For the
  symmetric taps of this lesson's design it makes no difference, which is
  exactly why the mistake survives until someone uses asymmetric taps. Exercise
  2 has to get this right when it walks the history oldest first.

- **Forgetting the delay.** A 31-tap symmetric filter delays everything by 15
  samples, half its length. Mixing its output with the unfiltered signal
  without that delay lines up the wrong samples.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Lesson 04's moving average is an FIR filter with every tap equal. Build an
   8-sample moving average from `fir.c`, and measure its gain at 1, 3, 6, 9 and
   12 kHz.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-moving-average/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. average.c ../../fir.c -o average -lm
   ./average
   ```

   ```
     1000 Hz:   -0.4 dB
     3000 Hz:   -4.0 dB
     6000 Hz: nothing gets through
     9000 Hz:  -13.1 dB
    12000 Hz: nothing gets through
   ```

   It is a low-pass filter, and a poor one. At 6 kHz, 8 samples hold exactly
   one cycle, which averages to zero; at 12 kHz, exactly two. But 9 kHz, in
   between, gets through at a fifth of its size. A moving average is good at
   smoothing and bad at separating frequencies, which is what designed taps are
   for. `-I ../..` tells the compiler to look for `fir.h` two folders up, so
   the exercise uses the lesson's own filter.

   </details>

2. `fir_process` checks at every tap whether to wrap around the ring. Store
   each input twice, `n` slots apart, so the last `n` inputs always sit side by
   side, and the inner loop needs no check. Is it faster?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-doubled/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined doubled.c -o doubled -lm
   ./doubled
   ```

   ```
   63 taps, 4800 samples: same output (within 1e-6): yes
   ```

   Timed, built with `-O2` and no sanitizers:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 doubled.c -o doubled -lm
   ```

   <!-- no-run -->

   ```bash
   ./doubled time
   ```

   With gcc 15, on the machine from lesson 19:

   ```
   63 taps, 4800 samples: same output (within 1e-6): yes
   ring:    29.70 ns per sample
   doubled: 15.93 ns per sample
   ```

   Almost twice as fast with gcc; clang already did better with the ring (20
   ns), and gained less. With the wrap gone, the inner loop of
   [`doubled.c`](exercises/ex2-doubled/doubled.c) is a plain dot product of two
   arrays, the shape compilers and processors are best at. The price is twice
   the memory for the history. The outputs agree to within 1e-6, not exactly,
   because the sum is added up in the opposite order, and floating-point
   addition gives slightly different results in different orders (lesson 26).

   </details>

3. Design the same 4 kHz low-pass with 31, 63 and 127 taps, and compare their
   gains between 3 and 6 kHz.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-more-taps/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. taps.c ../../fir.c -o taps -lm
   ./taps
   ```

   ```
   taps   3000 Hz  4000 Hz  5000 Hz  6000 Hz
     31      -1.9     -6.0    -14.3    -29.5
     63      -0.3     -6.0    -31.3    -65.9
    127       0.0     -6.0    -58.4    -62.2
   (gain in dB)
   ```

   Every version is −6 dB at the 4 kHz corner, but the more taps, the steeper
   the drop around it: at 5 kHz, 31 taps hold the signal back by 14 dB and 127
   taps by 58. Twice the taps, twice the work, and half the width of the
   transition. The 127-tap filter does no better than the 63-tap one at 6 kHz,
   though: how far the blocked band goes down is set by the window, and
   Hamming's limit is around −55 to −65 dB. A deeper stopband needs a different
   window, not just more taps.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                                  |
| ------------------------------------------------------------ | ------------ | ------------------------------------------------------------- |
| [`calloc`](https://en.cppreference.com/c/memory/calloc)      | `<stdlib.h>` | allocate memory for n elements, all bytes zero                |
| [`clock`](https://en.cppreference.com/c/chrono/clock)        | `<time.h>`   | processor time used by the program, in `CLOCKS_PER_SEC` units |
| [`cos`](https://en.cppreference.com/c/numeric/math/cos)      | `<math.h>`   | cosine of an angle in radians                                 |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)    | `<math.h>`   | absolute value of a `double`                                  |
| [`fabsf`](https://en.cppreference.com/c/numeric/math/fabs)   | `<math.h>`   | absolute value of a `float`                                   |
| [`free`](https://en.cppreference.com/c/memory/free)          | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc`         |
| [`log10`](https://en.cppreference.com/c/numeric/math/log10)  | `<math.h>`   | base-10 logarithm of a `double`                               |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)      | `<stdlib.h>` | allocate memory, contents unset                               |
| [`memcpy`](https://en.cppreference.com/c/string/byte/memcpy) | `<string.h>` | copy bytes between blocks that don't overlap                  |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                                  |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)      | `<math.h>`   | sine of an angle in radians                                   |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp) | `<string.h>` | compare two strings                                           |

<!-- /libfuncs -->
