# 21 · IIR filters: the biquad

Run this lesson's commands in `c-by-example/21-biquad/`.

Lesson 20's FIR filter looks only at inputs. An **infinite impulse response**
(IIR) filter also feeds back its own past *outputs*, and that feedback buys a
lot: a steep low-pass that needs dozens of FIR taps takes five numbers as an
IIR filter.

The standard building block is the **biquad**, which uses the last two inputs
and the last two outputs:

```
y[i] = b0·x[i] + b1·x[i-1] + b2·x[i-2] − a1·y[i-1] − a2·y[i-2]
```

Lesson 07 built its struct; this lesson designs the coefficients and runs it:

```bash
cat biquad.h
```

```c
/* biquad.h -- the biquad: a second-order IIR filter.
 *
 * An infinite impulse response (IIR) filter feeds its own past outputs
 * back in. A biquad uses two past inputs and two past outputs:
 *
 *   y[i] = b0 x[i] + b1 x[i-1] + b2 x[i-2] - a1 y[i-1] - a2 y[i-2]
 *
 * Five coefficients do what an FIR filter needs dozens of taps for.
 * The struct is public, as in lesson 07: it is small, holds no pointers,
 * and a caller can keep one on the stack or in an array, with no malloc.
 */
#ifndef BIQUAD_H
#define BIQUAD_H

#include <stddef.h>

/* What the filter IS. */
typedef struct biquad_coeffs
{
  float b0, b1, b2; /* feed-forward: how much of each input */
  float a1, a2;     /* feedback: how much of each past output */
} biquad_coeffs;

/* What it REMEMBERS: transposed direct form II needs two numbers. */
typedef struct biquad
{
  biquad_coeffs c;
  float         z1, z2;
} biquad_t;

/* Coefficients for a low-pass or high-pass filter with its corner at
   f0_hz, for a signal sampled at rate_hz. q sets the shape of the corner:
   0.7071 is the flattest passband (a Butterworth filter); higher q gives
   a peak just before the corner. Needs 0 < f0_hz < rate_hz / 2, q > 0. */
biquad_coeffs biquad_lowpass (double f0_hz, double q, double rate_hz);
biquad_coeffs biquad_highpass (double f0_hz, double q, double rate_hz);

/* Filter n samples from in into out; in and out may be the same. */
void biquad_process (biquad_t *f, const float *in, float *out, size_t n);

#endif /* BIQUAD_H */
```

```bash
cat biquad.c
```

```c
/* biquad.c -- design and run biquad filters. */
#include "biquad.h"

#include <math.h>

static const double PI = 3.141592653589793;

/* The low-pass and high-pass designs differ only in their b's, so both
   start here. These are the formulas of Robert Bristow-Johnson's "Audio
   EQ Cookbook", the usual source: they put the corner exactly at f0_hz. */
static biquad_coeffs
design (double f0_hz, double q, double rate_hz, double b0, double b1)
{
  double w0    = 2.0 * PI * f0_hz / rate_hz;
  double alpha = sin (w0) / (2.0 * q);
  double a0    = 1.0 + alpha;
  /* Every coefficient is divided by a0, so the a0 of the textbook's
     equation is 1 and drops out of it. */
  biquad_coeffs c = {
    .b0 = (float)(b0 / a0),
    .b1 = (float)(b1 / a0),
    .b2 = (float)(b0 / a0), /* b2 equals b0 in both designs */
    .a1 = (float)(-2.0 * cos (w0) / a0),
    .a2 = (float)((1.0 - alpha) / a0),
  };
  return c;
}

biquad_coeffs
biquad_lowpass (double f0_hz, double q, double rate_hz)
{
  double c = cos (2.0 * PI * f0_hz / rate_hz);
  return design (f0_hz, q, rate_hz, (1.0 - c) / 2.0, 1.0 - c);
}

biquad_coeffs
biquad_highpass (double f0_hz, double q, double rate_hz)
{
  double c = cos (2.0 * PI * f0_hz / rate_hz);
  return design (f0_hz, q, rate_hz, (1.0 + c) / 2.0, -(1.0 + c));
}

void
biquad_process (biquad_t *f, const float *in, float *out, size_t n)
{
  const biquad_coeffs c  = f->c;
  float               z1 = f->z1, z2 = f->z2;
  for (size_t i = 0; i < n; i++)
    {
      /* Transposed direct form II: see the README for how these three
         lines come from the equation in biquad.h. */
      float x = in[i];
      float y = c.b0 * x + z1;
      z1      = c.b1 * x - c.a1 * y + z2;
      z2      = c.b2 * x - c.a2 * y;
      out[i]  = y;
    }
  f->z1 = z1;
  f->z2 = z2;
}
```

## From the equation to three lines

Written as it reads, the equation keeps four numbers of memory: two past
inputs, two past outputs. That is **direct form I**, and `iir.c` below has it.
`biquad_process` uses a rearrangement that needs only two, `z1` and `z2`, each
holding the part of a *future* output that is already known:

- Output now is `b0·x` plus whatever was set aside for it: `y = b0·x + z1`.
- For the next sample, set aside its share from this sample (`b1·x − a1·y`),
  plus what the sample before set aside for two steps ahead (`z2`):
  `z1 = b1·x − a1·y + z2`.
- For two samples ahead, set aside its share from this one: `z2 = b2·x − a2·y`.

Expand `z1` and `z2` and the original equation comes back, term for term. This
is **transposed direct form II**: two numbers of state instead of four, and it
behaves well in floating point, which is why it is the usual choice.

## A low-pass, measured

```bash
cat iir.c
```

```c
/* iir.c -- a biquad low-pass, measured, and checked against the equation.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       iir.c biquad.c -o iir -lm && ./iir
 */
#include "biquad.h"

#include <math.h>
#include <stdio.h>

enum
{
  LEN  = 9600, /* 0.2 s at 48 kHz */
  SKIP = 4800  /* samples to let the filter settle before measuring */
};

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

static double
gain_db (biquad_coeffs c, double freq_hz)
{
  static float x[LEN];
  for (size_t i = 0; i < LEN; i++)
    x[i] = (float)sin (2.0 * PI * freq_hz * (double)i / RATE);
  biquad_t f = { .c = c }; /* state starts at zero */
  biquad_process (&f, x, x, LEN);
  float peak = 0.0f;
  for (size_t i = SKIP; i < LEN; i++)
    peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
  return 20.0 * log10 ((double)peak);
}

/* The equation in biquad.h, written out as it reads: DIRECT FORM I. It
   keeps four numbers of memory, the last two inputs and outputs. */
static void
direct_form_1 (biquad_coeffs c, const float *in, float *out, size_t n)
{
  float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
  for (size_t i = 0; i < n; i++)
    {
      float y = c.b0 * in[i] + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
      x2      = x1;
      x1      = in[i];
      y2      = y1;
      y1      = y;
      out[i]  = y;
    }
}

int
main (void)
{
  biquad_coeffs lp = biquad_lowpass (1000.0, 0.7071, RATE);
  printf ("b0 %.6f  b1 %.6f  b2 %.6f  a1 %.6f  a2 %.6f\n", (double)lp.b0,
          (double)lp.b1, (double)lp.b2, (double)lp.a1, (double)lp.a2);

  /* An impulse: the response dies away, but never reaches exactly 0. */
  static float imp[LEN], y[LEN], y1[LEN];
  imp[0]     = 1.0f;
  biquad_t f = { .c = lp };
  biquad_process (&f, imp, y, LEN);
  const size_t at[] = { 0, 1, 2, 10, 100 };
  printf ("impulse response:");
  for (size_t k = 0; k < sizeof at / sizeof at[0]; k++)
    printf (" [%zu] %.2e", at[k], (double)y[at[k]]);
  printf ("\n[1000] is below 1e-30: %s; exactly 0: %s\n",
          fabsf (y[1000]) < 1e-30f ? "yes" : "no",
          y[1000] == 0.0f ? "yes" : "no");

  /* Gain, an octave apart: past the corner, 12 dB less per octave. */
  const double freqs[] = { 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0 };
  for (size_t i = 0; i < sizeof freqs / sizeof freqs[0]; i++)
    printf ("%5.0f Hz: %7.2f dB\n", freqs[i], gain_db (lp, freqs[i]));

  /* The two forms compute the same filter. */
  direct_form_1 (lp, imp, y1, LEN);
  double worst = 0.0;
  for (size_t i = 0; i < LEN; i++)
    worst = fmax (worst, fabs ((double)y[i] - (double)y1[i]));
  printf ("direct form I agrees within 1e-6: %s\n",
          worst < 1e-6 ? "yes" : "no");
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined iir.c biquad.c -o iir -lm
./iir
```

```
b0 0.003916  b1 0.007832  b2 0.003916  a1 -1.815340  a2 0.831004
impulse response: [0] 3.92e-03 [1] 1.49e-02 [2] 2.78e-02 [10] 5.86e-02 [100] 2.59e-06
[1000] is below 1e-30: yes; exactly 0: no
  250 Hz:   -0.02 dB
  500 Hz:   -0.27 dB
 1000 Hz:   -3.01 dB
 2000 Hz:  -12.38 dB
 4000 Hz:  -24.60 dB
 8000 Hz:  -38.38 dB
direct form I agrees within 1e-6: yes
```

## What it shows

- **Infinite really means it.** An FIR filter's impulse response ends after its
  last tap. A biquad's rings on: a thousand samples later it is below 10⁻³⁰,
  and still not zero. Each output feeds the next, forever.
- **Five numbers, a steep filter.** Past the 1 kHz corner, each octave is 12 dB
  down (−12.4 at 2 kHz, −24.6 at 4 kHz), with five multiplies per sample.
  Lesson 20 needed 31 taps for a gentler slope than that.
- **The corner is −3 dB.** At `q = 0.7071` (1/√2) the filter is a *Butterworth*
  filter: as flat as possible below the corner, and down to half power, −3 dB,
  exactly at it.
- **Coefficients are computed once.** `biquad_lowpass` calls `sin` and `cos`
  when the filter is designed; `biquad_process` does only multiplies and adds,
  in a loop that never allocates, never fails, and touches nothing outside its
  arguments (lessons 13 and 18).
- **Two forms, one filter.** Direct form I and the transposed form agree to
  within 10⁻⁶ on a whole impulse response. Not exactly: they add the same
  products in a different order, and floating-point addition rounds differently
  in different orders.

## Pitfalls

- **Feedback can blow up.** If `a1` and `a2` are out of range, each echo comes
  back larger, and the output grows until it is `inf` (exercise 2). The design
  functions here always produce a stable filter; coefficients typed in by hand,
  or changed while the filter runs, may not. A biquad is stable when `|a2| < 1`
  and `|a1| < 1 + a2`.
- **Low corners in `float`.** A corner at 20 Hz with 48 kHz sampling puts `a1`
  within 0.003 of −2. A `float` has about seven significant digits, so its
  rounding moves the filter noticeably. Compute coefficients in `double` and
  run very low-frequency filters in `double` too.
- **Very small numbers can be slow.** As a filter rings down, its state passes
  into *subnormal* floats, below about 10⁻³⁸, which some processors handle far
  more slowly than ordinary ones (exercise 3).
- **Changing coefficients mid-stream.** Swapping in a new set while the state
  holds the old filter's memory can click, or briefly go unstable. Change them
  gradually, or between blocks with the state reset.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Put two biquads in series, with `q` = 0.5412 and 1.3066, and compare them
   with the single Butterworth biquad.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-cascade/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. cascade.c ../../biquad.c -o cascade -lm
   ./cascade
   ```

   ```
             one biquad   two biquads
     500 Hz:    -0.27 dB     -0.02 dB
    1000 Hz:    -3.01 dB     -3.01 dB
    2000 Hz:   -12.38 dB    -24.26 dB
    4000 Hz:   -24.60 dB    -48.99 dB
   ```

   Twice as steep: 24 dB per octave instead of 12, and flatter below the
   corner. Those two `q` values are the ones that make a pair of biquads into a
   *fourth-order* Butterworth filter: still exactly −3 dB at 1 kHz.
   Higher-order IIR filters are almost always built this way, as a chain of
   biquads, because one big high-order equation is far more sensitive to
   rounding. [`cascade.c`](exercises/ex1-cascade/cascade.c) runs each stage in
   place on the output of the last.

   </details>

2. Run an impulse through a biquad with `a1` = −1.9 and `a2` = 0.99, then with
   `a2` = 1.01. What happens to the output over 5000 samples?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-unstable/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. unstable.c ../../biquad.c -o unstable -lm
   ./unstable
   ```

   ```
   a2 = 0.99, stable; largest |y| per 1000 samples: 3e+00 2e-02 1e-04 9e-07 6e-09
   a2 = 1.01, unstable; largest |y| per 1000 samples: 2e+04 3e+06 5e+08 7e+10 1e+13
   ```

   With `a2` = 0.99 the ringing dies away, 100 times smaller every thousand
   samples. With 1.01 it grows 100 times larger instead, and keeps going: a few
   more seconds and it is `inf`, and after that every output is `inf` or `nan`.
   `a2` is the square of how much each cycle of the ringing is scaled by, so
   above 1, each cycle comes back bigger. A difference of 0.02 in one
   coefficient is the difference between a filter and an alarm.

   </details>

3. After an impulse, the biquad's output shrinks toward zero forever. Does it
   become subnormal, and does processing subnormal numbers cost more?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-denormals/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. denormal.c ../../biquad.c -o denormal -lm
   ./denormal
   ```

   ```
   output goes subnormal: yes, within 2000 samples: yes
   ```

   `fpclassify`, from `<math.h>`, says which kind of number a float is, and
   within 2000 samples of the impulse the output is `FP_SUBNORMAL`. To time it,
   [`denormal.c`](exercises/ex3-denormals/denormal.c) filters a constant input
   far too quiet to hear, once at 10⁻³⁰ (an ordinary float) and once at 10⁻⁴⁰
   (subnormal, so the filter's whole state stays subnormal):

   ```bash
   cc -std=c99 -Wall -Wextra -O2 -I ../.. denormal.c ../../biquad.c -o denormal -lm
   ```

   <!-- no-run -->

   ```bash
   ./denormal time
   ```

   On the machine from lesson 19:

   ```
   output goes subnormal: yes, within 2000 samples: yes
   input at 1e-30 (normal):     2.55 ns per sample
   input at 1e-40 (subnormal):  2.80 ns per sample
   ```

   About 10% slower on this recent AMD processor, which handles subnormals
   mostly in hardware. Many processors, older Intel ones in particular, take
   tens of times longer per subnormal operation, and an effects chain gone
   quiet could miss its deadline in exactly the moment it seemed to have
   nothing to do. That is why audio code often sets the processor to flush
   subnormals to zero, or adds a tiny offset to keep the state normal. Measure
   on the processors you ship to.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                              | header       | what it does                                                            |
| --------------------------------------------------------------------- | ------------ | ----------------------------------------------------------------------- |
| [`clock`](https://en.cppreference.com/c/chrono/clock)                 | `<time.h>`   | processor time used by the program, in `CLOCKS_PER_SEC` units           |
| [`cos`](https://en.cppreference.com/c/numeric/math/cos)               | `<math.h>`   | cosine of an angle in radians                                           |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)             | `<math.h>`   | absolute value of a `double`                                            |
| [`fabsf`](https://en.cppreference.com/c/numeric/math/fabs)            | `<math.h>`   | absolute value of a `float`                                             |
| [`fmax`](https://en.cppreference.com/c/numeric/math/fmax)             | `<math.h>`   | the larger of two numbers (and not a NaN, if one is)                    |
| [`fpclassify`](https://en.cppreference.com/c/numeric/math/fpclassify) | `<math.h>`   | what kind of number: zero, subnormal, normal, infinite or NaN (a macro) |
| [`log10`](https://en.cppreference.com/c/numeric/math/log10)           | `<math.h>`   | base-10 logarithm of a `double`                                         |
| [`printf`](https://en.cppreference.com/c/io/fprintf)                  | `<stdio.h>`  | formatted output to `stdout`                                            |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)               | `<math.h>`   | sine of an angle in radians                                             |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp)          | `<string.h>` | compare two strings                                                     |

<!-- /libfuncs -->
