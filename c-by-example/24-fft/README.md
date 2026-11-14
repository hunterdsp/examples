# 24 · The FFT

Run this lesson's commands in `c-by-example/24-fft/`.

Lesson 23 measured one frequency in a signal by multiplying it by a phasor and
adding up. Doing that for every frequency at once is the **discrete Fourier
transform** (DFT): `n` samples in, `n` complex numbers out, one per frequency
*bin*. Bin `k` says how much of the frequency `k` cycles per `n` samples is
present, and at what angle. It is how a spectrum analyser draws its picture,
how a filter's response is measured, and the first step of most audio
compression.

Done directly, the DFT costs `n × n` complex multiplies: a million for 1024
samples. The **fast Fourier transform** (FFT) finds exactly the same numbers in
about `n/2 × log₂ n`: 5120. It splits the transform into two half-size ones
(the even samples and the odd), splits those again, all the way down to pairs,
and combines the halves on the way back up with a step called a **butterfly**.

The interface follows lesson 13: a *plan* does everything that depends only on
`n`, once, and the transform itself never allocates.

```bash
cat fft.h
```

```c
/* fft.h -- the fast Fourier transform, radix 2.
 *
 * The discrete Fourier transform (DFT) of n samples is n complex numbers,
 * one per frequency: bin k says how much of the frequency k cycles per n
 * samples is in the signal, and at what angle (lesson 23's measurement,
 * for every k at once). Done directly, that is n * n multiplies. The FFT
 * gets the same answer in about n log2 n, for n a power of two.
 *
 * Setup and use are separate, as lesson 13 asks: fft_plan_create
 * allocates and computes everything that depends only on n, once;
 * fft_forward then allocates nothing.
 */
#ifndef FFT_H
#define FFT_H

#include <complex.h>
#include <stddef.h>

typedef struct fft_plan fft_plan;

/* A plan for n-point transforms. NULL if n is not a power of two (2, 4,
   8, ...) or memory is short. Free it with fft_plan_destroy. */
fft_plan *fft_plan_create (size_t n);

void fft_plan_destroy (fft_plan *p);

/* The plan's n. */
size_t fft_size (const fft_plan *p);

/* Transform x, which holds fft_size (p) samples, in place: afterwards
   x[k] is bin k. */
void fft_forward (const fft_plan *p, double complex *x);

#endif /* FFT_H */
```

```bash
cat fft.c
```

```c
/* fft.c -- an iterative radix-2 FFT. */
#include "fft.h"

#include <math.h>
#include <stdlib.h>

struct fft_plan
{
  size_t         n;
  double complex tw[]; /* n / 2 twiddle factors: e^(-2 pi i k / n) */
};

static const double PI = 3.141592653589793;

fft_plan *
fft_plan_create (size_t n)
{
  if (n < 2 || (n & (n - 1)) != 0) /* a power of two has one bit set */
    return NULL;
  fft_plan *p = malloc (sizeof *p + n / 2 * sizeof p->tw[0]);
  if (p == NULL)
    return NULL;
  p->n = n;
  /* Each from cexp, not by multiplying one phasor by itself, which
     drifts (lesson 23, exercise 3). */
  for (size_t k = 0; k < n / 2; k++)
    p->tw[k] = cexp (-2.0 * PI * I * (double)k / (double)n);
  return p;
}

void
fft_plan_destroy (fft_plan *p)
{
  free (p);
}

size_t
fft_size (const fft_plan *p)
{
  return p->n;
}

void
fft_forward (const fft_plan *p, double complex *x)
{
  size_t n = p->n;

  /* 1. Reorder: put x[i] at the index whose bits are i's bits reversed.
        After this, every pair, quad, ... the stages below combine sits
        side by side. */
  for (size_t i = 1, j = 0; i < n; i++)
    {
      size_t bit = n >> 1;
      for (; j & bit; bit >>= 1) /* add 1 to j, counting from the top */
        j ^= bit;
      j ^= bit;
      if (i < j)
        {
          double complex t = x[i];
          x[i]             = x[j];
          x[j]             = t;
        }
    }

  /* 2. Combine: transforms of length len/2 become transforms of length
        len, by BUTTERFLIES. Each takes one value a from the first half
        and one b from the second, turns b by a twiddle factor, and
        writes a + b and a - b. log2 n stages of n / 2 butterflies. */
  for (size_t len = 2; len <= n; len <<= 1)
    {
      size_t half = len / 2;
      size_t step = n / len; /* twiddle k for this length is tw[k*step] */
      for (size_t start = 0; start < n; start += len)
        for (size_t k = 0; k < half; k++)
          {
            double complex a    = x[start + k];
            double complex b    = x[start + k + half] * p->tw[k * step];
            x[start + k]        = a + b;
            x[start + k + half] = a - b;
          }
    }
}
```

A program that transforms two tones, and checks the FFT against the DFT done
straight from its definition:

```bash
cat spectrum.c
```

```c
/* spectrum.c -- the spectrum of a signal, by FFT, checked by DFT.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       spectrum.c fft.c -o spectrum -lm && ./spectrum
 *
 * Two tones, transformed. The FFT's answer is compared with the DFT's,
 * computed straight from its definition, the slow way.
 */
#include "fft.h"

#include <math.h>
#include <stdio.h>

enum
{
  N = 1024
};

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

/* The DFT by its definition: bin k is the sum of x[i] e^(-2 pi i k i/n). */
static void
dft (const double complex *x, double complex *out, size_t n)
{
  for (size_t k = 0; k < n; k++)
    {
      double complex sum = 0.0;
      for (size_t i = 0; i < n; i++)
        sum += x[i] * cexp (-2.0 * PI * I * (double)((k * i) % n) / (double)n);
      out[k] = sum;
    }
}

int
main (void)
{
  fft_plan *p = fft_plan_create (N);
  if (p == NULL)
    return 1;

  /* Bin k is k * RATE / N Hz: 46.875 Hz apart. Two tones that land
     exactly on bins 20 and 64: 937.5 Hz, size 0.5, and 3000 Hz, 0.2. */
  static double complex x[N], slow[N];
  for (size_t i = 0; i < N; i++)
    x[i] = 0.5 * cos (2.0 * PI * 20.0 * (double)i / (double)N)
           + 0.2 * cos (2.0 * PI * 64.0 * (double)i / (double)N);
  dft (x, slow, N);
  fft_forward (p, x);

  double worst = 0.0;
  for (size_t k = 0; k < N; k++)
    worst = fmax (worst, cabs (x[k] - slow[k]));
  printf ("FFT and DFT agree within 1e-9: %s\n", worst < 1e-9 ? "yes" : "no");

  /* Bins up to N/2 (half the sample rate) are the frequencies; the rest
     mirror them, for a real signal. A cosine of size A shows as N*A/2. */
  printf ("bins with anything in them, up to %.0f Hz:\n", RATE / 2.0);
  for (size_t k = 0; k <= N / 2; k++)
    {
      double size = cabs (x[k]) * 2.0 / (double)N;
      if (size > 1e-6)
        printf ("  bin %3zu = %6.1f Hz: size %.4f\n", k,
                (double)k * RATE / (double)N, size);
    }

  printf ("complex multiplies: DFT %zu, FFT %zu\n", (size_t)N * N,
          (size_t)N / 2 * 10); /* log2 1024 = 10 stages */
  fft_plan_destroy (p);
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined spectrum.c fft.c -o spectrum -lm
./spectrum
```

```
FFT and DFT agree within 1e-9: yes
bins with anything in them, up to 24000 Hz:
  bin  20 =  937.5 Hz: size 0.5000
  bin  64 = 3000.0 Hz: size 0.2000
complex multiplies: DFT 1048576, FFT 5120
```

## What it shows

- **Bins are evenly spaced.** With 1024 samples at 48 kHz, bin `k` is
  `k × 48000 / 1024` Hz: 46.875 Hz apart. The two tones were chosen to land
  exactly on bins 20 and 64, so each shows up in one bin, at its full size, and
  every other bin is empty.
- **Only half the bins are new.** For a real signal, bins above `n/2` mirror
  the ones below (the negative frequencies of lesson 23's cosine), so the
  spectrum stops at bin 512, half the sample rate.
- **The two passes.** First the samples are reordered by *bit reversal*: sample
  1 (`0000000001`) moves to 512 (`1000000000`), and so on. That puts every pair
  the first stage combines side by side, and every quad the second combines,
  and so on. Then ten stages of 512 butterflies each: take `a` and `b`, turn
  `b` by a twiddle factor, and write `a + b` and `a − b`.
- **Twiddles from the plan.** Every twiddle factor is one of the `n/2` phasors
  `e^(−2πik/n)`, computed once in `fft_plan_create`, each straight from `cexp`
  so none of them drifts (lesson 23, exercise 3). A stage of length `len` uses
  every `n/len`-th one.
- **Check against the definition.** The FFT is easy to get subtly wrong, and
  the DFT is easy to get right. They agree to within 10⁻⁹, which is
  floating-point rounding: the same answer, by a different route.

## Pitfalls

- **The plan and the buffer disagree.**
  [`broken-size-mismatch.c`](broken-size-mismatch.c) runs a 1024-point
  reordering over a buffer of 1000 samples. It swaps samples anywhere up to
  index 1023, and the sanitizers stop it at the first one past the end:

  ```
  broken-size-mismatch.c:26:31: runtime error: load of address 0x6e8f405e4000 with insufficient space for an object of type 'complex double'
  ```

  The size comes from the plan, so every buffer handed to `fft_forward` must
  have exactly that many samples. A block of 1000 gets padded with zeros to
  1024\.

- **Only powers of two.** This FFT halves the problem at every stage, so `n`
  must be 2, 4, 8, …, and `fft_plan_create` returns `NULL` otherwise. FFTs for
  other sizes exist, and libraries like FFTW provide them; a radix-2 one is the
  place to start.

- **Leakage.** A tone that isn't exactly on a bin spreads into all of them
  (exercise 3). Real signals are never exactly on a bin.

- **Scaling conventions.** This FFT doesn't divide by anything, so a cosine of
  size `A` shows up as `n·A/2`. Other libraries divide by `n` or `√n`. Check
  which one you have before comparing numbers.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. The inverse DFT turns a spectrum back into samples: the same sum, with the
   phasors turning the other way, divided by `n`. Write it using `fft_forward`
   and nothing else, and check that a signal survives the round trip.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-inverse/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. inverse.c ../../fft.c -o inverse -lm
   ./inverse
   ```

   ```
   forward then inverse: back within 1e-12: yes
   ```

   Turning the other way is what the conjugate does, so
   [`fft_inverse`](exercises/ex1-inverse/inverse.c) conjugates the input, runs
   the forward FFT, conjugates the result and divides by `n`. One transform,
   two directions: filtering in the frequency domain is a forward FFT, a
   multiply per bin, and an inverse.

   </details>

2. Time the DFT and the FFT at 256, 1024 and 4096 points.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-speed/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. speed.c ../../fft.c -o speed -lm
   ./speed
   ```

   ```
   n =  256: FFT matches DFT: yes
   ```

   Built for timing:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 -I ../.. speed.c ../../fft.c -o speed -lm
   ```

   <!-- no-run -->

   ```bash
   ./speed time
   ```

   On the machine from lesson 19:

   ```
   n =  256: FFT matches DFT: yes; DFT    1.178 ms, FFT  0.002 ms
   n = 1024: FFT matches DFT: yes; DFT   19.778 ms, FFT  0.006 ms
   n = 4096: FFT matches DFT: yes; DFT  260.829 ms, FFT  0.032 ms
   ```

   Each time `n` goes up 4 times, the DFT takes about 16 times longer, as `n²`
   says, while the FFT takes only about 5 times longer. At 4096 points, the FFT
   is 8000 times as fast, and the gap keeps widening. This is what made
   real-time spectrum analysis possible, and a lesson 19 of its own: the best
   optimisation is usually a better algorithm, not a faster loop.

   </details>

3. Transform a 1000 Hz tone, which falls between bins 21 and 22, with and
   without a *Hann window*: the block multiplied by a curve that fades it in
   from 0 and out again. Compare the bins around it.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-window/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. window.c ../../fft.c -o window -lm
   ./window
   ```

   ```
   bin     Hz     no window   Hann window
    10   468.75     -33.0 dB      -74.6 dB
    18   843.75     -21.9 dB      -41.7 dB
    20   937.50     -13.8 dB      -11.5 dB
    21   984.38      -1.7 dB       -0.6 dB
    22  1031.25      -7.6 dB       -2.6 dB
    24  1125.00     -19.5 dB      -35.4 dB
    32  1500.00     -31.0 dB      -72.8 dB
   ```

   The FFT treats its block as one cycle of a signal that repeats forever.
   21.33 cycles of 1000 Hz don't end where they started, so the repeats jump at
   every join, and a jump contains every frequency: without a window, bins far
   from 1000 Hz still show −31 to −33 dB. The Hann window brings both ends of
   the block smoothly to 0, so there is no jump: 40 dB less leakage 500 Hz
   away. The price is a wider peak, since bins 20 and 22 now hold more of the
   tone. Every spectrum analyser windows its blocks for this reason.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                       | header        | what it does                                                  |
| -------------------------------------------------------------- | ------------- | ------------------------------------------------------------- |
| [`cabs`](https://en.cppreference.com/c/numeric/complex/cabs)   | `<complex.h>` | size (magnitude) of a `double complex`                        |
| [`calloc`](https://en.cppreference.com/c/memory/calloc)        | `<stdlib.h>`  | allocate memory for n elements, all bytes zero                |
| [`cexp`](https://en.cppreference.com/c/numeric/complex/cexp)   | `<complex.h>` | *e* raised to a complex power: `cexp (I * t)` is a phasor     |
| [`clock`](https://en.cppreference.com/c/chrono/clock)          | `<time.h>`    | processor time used by the program, in `CLOCKS_PER_SEC` units |
| [`conj`](https://en.cppreference.com/c/numeric/complex/conj)   | `<complex.h>` | complex conjugate: the imaginary part negated                 |
| [`cos`](https://en.cppreference.com/c/numeric/math/cos)        | `<math.h>`    | cosine of an angle in radians                                 |
| [`creal`](https://en.cppreference.com/c/numeric/complex/creal) | `<complex.h>` | real part of a `double complex`                               |
| [`exp`](https://en.cppreference.com/c/numeric/math/exp)        | `<math.h>`    | *e* raised to the power x                                     |
| [`fmax`](https://en.cppreference.com/c/numeric/math/fmax)      | `<math.h>`    | the larger of two numbers (and not a NaN, if one is)          |
| [`free`](https://en.cppreference.com/c/memory/free)            | `<stdlib.h>`  | give back memory from `malloc`, `calloc` or `realloc`         |
| [`log10`](https://en.cppreference.com/c/numeric/math/log10)    | `<math.h>`    | base-10 logarithm of a `double`                               |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)        | `<stdlib.h>`  | allocate memory, contents unset                               |
| [`memcpy`](https://en.cppreference.com/c/string/byte/memcpy)   | `<string.h>`  | copy bytes between blocks that don't overlap                  |
| [`printf`](https://en.cppreference.com/c/io/fprintf)           | `<stdio.h>`   | formatted output to `stdout`                                  |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)        | `<math.h>`    | sine of an angle in radians                                   |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp)   | `<string.h>`  | compare two strings                                           |

<!-- /libfuncs -->
