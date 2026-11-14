/* window.c -- a tone between two bins, with and without a window.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       window.c ../../fft.c -o window -lm && ./window
 *
 * 1000 Hz is 21.33 bins at 48 kHz with 1024 points: no bin is exactly
 * on it. The FFT treats its 1024 samples as one period of a signal that
 * repeats forever, and 21.33 cycles don't join up end to start, so the
 * jump at the join spreads energy into every bin: LEAKAGE. A WINDOW
 * fades the block in and out, so there is no jump.
 */
#include "fft.h"

#include <math.h>
#include <stdio.h>

enum
{
  N = 1024
};

static const double PI = 3.141592653589793;

int
main (void)
{
  fft_plan *p = fft_plan_create (N);
  if (p == NULL)
    return 1;
  static double complex plain[N], hann[N];
  double gain = 0.0; /* the window's average: its effect on size */
  for (size_t i = 0; i < N; i++)
    {
      double x = cos (2.0 * PI * 1000.0 * (double)i / 48000.0);
      double w = 0.5 - 0.5 * cos (2.0 * PI * (double)i / (double)N); /* Hann */
      plain[i] = x;
      hann[i]  = x * w;
      gain += w / (double)N;
    }
  fft_forward (p, plain);
  fft_forward (p, hann);

  printf ("bin     Hz     no window   Hann window\n");
  const size_t bins[] = { 10, 18, 20, 21, 22, 24, 32 };
  for (size_t j = 0; j < sizeof bins / sizeof bins[0]; j++)
    {
      size_t k = bins[j];
      double a = cabs (plain[k]) * 2.0 / (double)N;
      double b = cabs (hann[k]) * 2.0 / (double)N / gain;
      printf ("%3zu %8.2f  %8.1f dB   %8.1f dB\n", k, (double)k * 46.875,
              20.0 * log10 (a), 20.0 * log10 (b));
    }
  fft_plan_destroy (p);
  return 0;
}
