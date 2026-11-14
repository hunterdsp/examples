# 23 · Complex numbers

Run this lesson's commands in `c-by-example/23-complex/`.

A sine wave has two things a single number can't hold at once: how big it is,
and where in its cycle it is. A **complex number** holds both. It has a real
part and an imaginary part, written `a + bi`, where `i` is a number with
`i·i = −1`; drawn on a plane, it is a point with a *size* (its distance from 0)
and an *angle*.

The useful complex numbers in signal processing are **phasors**: size 1, at
some angle θ, written `e^(iθ)` (that is `cos θ + i sin θ`, by Euler's formula).
Multiplying by a phasor rotates by its angle, and a phasor turning by the same
angle every sample is a single, pure frequency. Filters, mixers and the FFT
(lesson 24) are all built from them.

C99 has complex numbers built in, in `<complex.h>`: `double complex` is a type,
`I` is `i`, and `+`, `−`, `*` and `/` work on them.

```bash
cat phasor.c
```

```c
/* phasor.c -- complex numbers, and why signal processing lives on them.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       phasor.c -o phasor -lm && ./phasor
 *
 * C99's <complex.h> adds complex types: `double complex` holds a real
 * and an imaginary part, and I is the imaginary unit, i, with i*i = -1.
 * Arithmetic works on them directly. A complex number of size 1 at angle
 * theta, e^(i theta), is a PHASOR: multiplying by it rotates, and a
 * phasor turning by the same angle each sample is a pure frequency.
 */
#include <complex.h>
#include <math.h>
#include <stdio.h>

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

enum
{
  N = 4800 /* 0.1 s: a whole number of cycles of every tone below */
};

/* The size and angle (in degrees) of the tone at freq_hz in x: correlate
   with a phasor turning the other way, so that tone stops turning and
   adds up, while every other frequency keeps turning and cancels. This
   is one bin of a discrete Fourier transform (lesson 24). */
static double complex
measure (const float *x, size_t n, double freq_hz)
{
  double complex sum = 0.0;
  for (size_t i = 0; i < n; i++)
    sum += (double)x[i] * cexp (-I * 2.0 * PI * freq_hz * (double)i / RATE);
  return sum * (2.0 / (double)n); /* scaled so a cosine of size A gives A */
}

/* Parts smaller than the last printed digit are rounding error: show
   them as 0, so one machine's -0.0000 doesn't differ from another's. */
static double
tidy (double v)
{
  return fabs (v) < 5e-5 ? 0.0 : v;
}

static void
show (const char *label, double complex z)
{
  z = tidy (creal (z)) + tidy (cimag (z)) * I;
  printf ("%-16s %7.4f %+7.4fi", label, creal (z), cimag (z));
  if (cabs (z) < 5e-5) /* no size, so no meaningful angle */
    printf ("   size 0: no tone here\n");
  else
    printf ("   size %.4f, angle %6.1f deg\n", cabs (z),
            carg (z) * 180.0 / PI);
}

int
main (void)
{
  double complex z = 3.0 + 4.0 * I;
  show ("z = 3 + 4i:", z);
  show ("conj (z):", conj (z));
  show ("i * i:", I * I);
  show ("z * i:", z * I); /* times i: a quarter turn */
  show ("e^(i pi/2):", cexp (I * PI / 2.0));

  /* A 1 kHz cosine of size 0.5, started 30 degrees into its cycle, plus
     a 3 kHz one of size 0.2, added together in one signal. */
  static float x[N];
  for (size_t i = 0; i < N; i++)
    {
      double t = (double)i / RATE;
      x[i]     = (float)(0.5 * cos (2.0 * PI * 1000.0 * t + PI / 6.0)
                         + 0.2 * cos (2.0 * PI * 3000.0 * t));
    }
  show ("tone at 1000 Hz:", measure (x, N, 1000.0));
  show ("tone at 3000 Hz:", measure (x, N, 3000.0));
  show ("tone at 2000 Hz:", measure (x, N, 2000.0));
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined phasor.c -o phasor -lm
./phasor
```

```
z = 3 + 4i:       3.0000 +4.0000i   size 5.0000, angle   53.1 deg
conj (z):         3.0000 -4.0000i   size 5.0000, angle  -53.1 deg
i * i:           -1.0000 +0.0000i   size 1.0000, angle  180.0 deg
z * i:           -4.0000 +3.0000i   size 5.0000, angle  143.1 deg
e^(i pi/2):       0.0000 +1.0000i   size 1.0000, angle   90.0 deg
tone at 1000 Hz:  0.4330 +0.2500i   size 0.5000, angle   30.0 deg
tone at 3000 Hz:  0.2000 +0.0000i   size 0.2000, angle    0.0 deg
tone at 2000 Hz:  0.0000 +0.0000i   size 0: no tone here
```

## What it shows

- **Size and angle.** `cabs` gives the size (3, 4, 5: Pythagoras) and `carg`
  the angle, in radians. `conj` mirrors the number across the real axis.
- **Multiplying by `i` is a quarter turn.** `z * i` has the same size as `z`
  and an angle 90° larger, and `i * i` is a half turn, to −1. In general,
  multiplying complex numbers *multiplies their sizes and adds their angles*,
  which is what makes them the natural language for things that rotate.
- **Measuring a tone.** `measure` multiplies the signal by a phasor turning
  backwards at the frequency asked about, and adds up the products. A tone at
  exactly that frequency stops turning and adds up to its size and starting
  angle: 0.5 and 30°, as it was made. The 3 kHz tone keeps turning, so its
  products cancel out over whole cycles, and the 2 kHz measurement finds
  nothing at all. Each tone is found on its own, though all three are in one
  signal.
- **The measurement is the DFT.** Doing this for every frequency at once is the
  *discrete Fourier transform*, and lesson 24 makes it fast.

## Pitfalls

- **`printf` has no complex conversion.**
  [`broken-complex-printf.c`](broken-complex-printf.c) prints a
  `double complex` with `%f`, which reads one `double` where there are two.
  `-Wall` checks formats against arguments and refuses:

  ```
  broken-complex-printf.c:15:17: error: format ‘%f’ expects argument of type ‘double’, but argument 2 has type ‘complex double’ [-Werror=format=]
  ```

  Print the parts, `creal (z)` and `cimag (z)`.

- **Not every compiler has it.** Microsoft's C compiler has never supported
  `<complex.h>`'s types, and C11 made them optional. Code that must build
  everywhere brings its own complex struct (exercise 2).

- **`float complex` needs the `f` functions.** `cabs` takes a `double complex`;
  for a `float complex` it is `cabsf`, as `sqrtf` is to `sqrt`. Mixing them
  converts back and forth silently.

- **Rounding piles up in a rotation.** A phasor multiplied by itself sample
  after sample slowly changes size (exercise 3).

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. A *mixer* shifts a signal's frequency by multiplying it with an oscillator.
   Shift a complex 1100 Hz tone down 1000 Hz with a phasor, and a real 1100 Hz
   tone with a cosine. What frequencies come out of each?

   <details><summary>Output and answer</summary>

   In `exercises/ex1-mixer/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined mixer.c -o mixer -lm
   ./mixer
   ```

   ```
           -2100 Hz  -100 Hz  +100 Hz  +2100 Hz
   phasor:     0.00     0.00     1.00     0.00
   cosine:     0.25     0.25     0.25     0.25
   ```

   The phasor moves the tone to +100 Hz and nowhere else: multiplying adds the
   angles, so 1100 − 1000 = 100, exactly. The cosine produces 100 Hz *and* 2100
   Hz, each as a pair of phasors turning both ways, because a real cosine is
   itself two phasors, one turning each way, and each one shifts the tone its
   own way. That second copy has to be filtered out afterwards. Complex signals
   carry the direction of turning, which is why software radios work with
   complex samples.

   </details>

2. Write lesson 23's tone measurement without `<complex.h>`, with a struct of
   two `double`s and functions to multiply and add them. Does it agree?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-struct/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined cplx.c -o cplx -lm
   ./cplx
   ```

   ```
   struct:      0.4330 +0.2500i
   <complex.h>: 0.4330 +0.2500i
   agree within 1e-12: yes
   ```

   [`cplx.c`](exercises/ex2-struct/cplx.c) needs three functions: `cmul` (from
   `i·i = −1`), `cadd`, and `cpolar` (Euler's formula). The arithmetic is the
   same, only spelled out, and the results agree to 12 decimal places. It is
   more to write, and it compiles everywhere, MSVC included, which is why many
   portable libraries take this route.

   </details>

3. A phasor multiplied by `e^(i 2π f / rate)` every sample turns at `f`: an
   oscillator with one multiply and no `sin`. Run one in `float complex` for 10
   million samples. Does its size stay 1?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-rotator/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined rotator.c -o rotator -lm
   ./rotator
   ```

   ```
   after 10000000 steps:
   left alone:   size off 1 by more than 0.001: yes
   renormalised: size off 1 by less than 1e-5:  yes
   ```

   It doesn't: each multiply rounds, the size creeps away from 1, and nothing
   pulls it back (here it ends about 16% small; on another machine the drift
   may differ). The fix is to measure the size now and then and scale it back.
   [`rotator.c`](exercises/ex3-rotator/rotator.c) multiplies by
   `(3 − |z|²) / 2` every 1000 samples, a cheap stand-in for `1 / |z|` when
   `|z|` is close to 1, with no square root. A phase accumulator (lesson 22)
   has no such drift, since integers don't round; a rotator is cheaper when a
   complex output is needed anyway.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                        | header        | what it does                                              |
| --------------------------------------------------------------- | ------------- | --------------------------------------------------------- |
| [`cabs`](https://en.cppreference.com/c/numeric/complex/cabs)    | `<complex.h>` | size (magnitude) of a `double complex`                    |
| [`cabsf`](https://en.cppreference.com/c/numeric/complex/cabs)   | `<complex.h>` | size of a `float complex`                                 |
| [`carg`](https://en.cppreference.com/c/numeric/complex/carg)    | `<complex.h>` | angle of a complex number, in radians                     |
| [`cexp`](https://en.cppreference.com/c/numeric/complex/cexp)    | `<complex.h>` | *e* raised to a complex power: `cexp (I * t)` is a phasor |
| [`cimag`](https://en.cppreference.com/c/numeric/complex/cimag)  | `<complex.h>` | imaginary part of a `double complex`                      |
| [`cimagf`](https://en.cppreference.com/c/numeric/complex/cimag) | `<complex.h>` | imaginary part of a `float complex`                       |
| [`conj`](https://en.cppreference.com/c/numeric/complex/conj)    | `<complex.h>` | complex conjugate: the imaginary part negated             |
| [`cos`](https://en.cppreference.com/c/numeric/math/cos)         | `<math.h>`    | cosine of an angle in radians                             |
| [`creal`](https://en.cppreference.com/c/numeric/complex/creal)  | `<complex.h>` | real part of a `double complex`                           |
| [`crealf`](https://en.cppreference.com/c/numeric/complex/creal) | `<complex.h>` | real part of a `float complex`                            |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)       | `<math.h>`    | absolute value of a `double`                              |
| [`fabsf`](https://en.cppreference.com/c/numeric/math/fabs)      | `<math.h>`    | absolute value of a `float`                               |
| [`printf`](https://en.cppreference.com/c/io/fprintf)            | `<stdio.h>`   | formatted output to `stdout`                              |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)         | `<math.h>`    | sine of an angle in radians                               |

<!-- /libfuncs -->
