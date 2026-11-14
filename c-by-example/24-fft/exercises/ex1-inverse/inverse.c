/* inverse.c -- back from the spectrum, with the forward FFT.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       inverse.c ../../fft.c -o inverse -lm && ./inverse
 *
 * The inverse DFT is the forward one with the phasors turning the other
 * way, divided by n. Turning the other way is what conjugating does, so
 *
 *   inverse (X) = conj (forward (conj (X))) / n
 *
 * and one forward FFT serves both directions.
 */
#include "fft.h"

#include <math.h>
#include <stdio.h>

enum
{
  N = 256
};

static void
fft_inverse (const fft_plan *p, double complex *x)
{
  size_t n = fft_size (p);
  for (size_t i = 0; i < n; i++)
    x[i] = conj (x[i]);
  fft_forward (p, x);
  for (size_t i = 0; i < n; i++)
    x[i] = conj (x[i]) / (double)n;
}

int
main (void)
{
  fft_plan *p = fft_plan_create (N);
  if (p == NULL)
    return 1;
  static double complex x[N], orig[N];
  for (size_t i = 0; i < N; i++) /* any signal will do: a decaying chirp */
    orig[i] = x[i] = exp (-(double)i / 64.0) * sin (0.001 * (double)(i * i));

  fft_forward (p, x);
  fft_inverse (p, x);
  double worst = 0.0;
  for (size_t i = 0; i < N; i++)
    worst = fmax (worst, cabs (x[i] - orig[i]));
  printf ("forward then inverse: back within 1e-12: %s\n",
          worst < 1e-12 ? "yes" : "no");
  fft_plan_destroy (p);
  return 0;
}
