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
