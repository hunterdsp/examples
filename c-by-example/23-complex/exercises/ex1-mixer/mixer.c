/* mixer.c -- moving a tone down in frequency, two ways.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       mixer.c -o mixer -lm && ./mixer
 *
 * MIXING multiplies a signal by an oscillator, and shifts its frequency.
 * Multiplying by a phasor turning at -1000 Hz shifts everything down by
 * exactly 1000 Hz. Multiplying a real tone by a real cosine shifts it
 * down AND up, because a cosine is two phasors, one turning each way.
 */
#include <complex.h>
#include <math.h>
#include <stdio.h>

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

enum
{
  N = 4800
};

/* How much of the phasor at freq_hz (negative: turning backwards) is in
   y, as a size: 1 for a phasor of size 1. */
static double
amount (const double complex *y, double freq_hz)
{
  double complex sum = 0.0;
  for (size_t i = 0; i < N; i++)
    sum += y[i] * cexp (-I * 2.0 * PI * freq_hz * (double)i / RATE);
  return cabs (sum) / (double)N;
}

int
main (void)
{
  static double complex by_phasor[N], by_cosine[N];
  for (size_t i = 0; i < N; i++)
    {
      double w1100 = 2.0 * PI * 1100.0 * (double)i / RATE;
      double w1000 = 2.0 * PI * 1000.0 * (double)i / RATE;
      /* A complex tone, times a phasor turning backwards at 1000 Hz. */
      by_phasor[i] = cexp (I * w1100) * cexp (-I * w1000);
      /* A real tone, times a real cosine at 1000 Hz. */
      by_cosine[i] = cos (w1100) * cos (w1000);
    }
  const double freqs[] = { -2100.0, -100.0, 100.0, 2100.0 };
  printf ("        -2100 Hz  -100 Hz  +100 Hz  +2100 Hz\n");
  printf ("phasor:");
  for (size_t k = 0; k < 4; k++)
    printf ("  %7.2f", amount (by_phasor, freqs[k]));
  printf ("\ncosine:");
  for (size_t k = 0; k < 4; k++)
    printf ("  %7.2f", amount (by_cosine, freqs[k]));
  printf ("\n");
  return 0;
}
