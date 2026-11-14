/* taps.c -- the same low-pass design at 31, 63 and 127 taps.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       taps.c ../../fir.c -o taps -lm && ./taps
 *
 * More taps buy a sharper filter: the drop from passing to blocking
 * happens over a narrower band. They cost one multiply-add each, for
 * every sample.
 */
#include "fir.h"

#include <math.h>
#include <stdio.h>

enum
{
  MAX_TAPS = 127,
  LEN      = 4800
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
   the output's peak, once the first n taps' worth of samples have passed,
   with 1. */
static double
gain_db (fir_t *f, size_t n, double freq_hz)
{
  static float x[LEN];
  for (size_t i = 0; i < LEN; i++)
    x[i] = (float)sin (2.0 * PI * freq_hz * (double)i / RATE);
  fir_reset (f);
  fir_process (f, x, x, LEN); /* in place */
  float peak = 0.0f;
  for (size_t i = n; i < LEN; i++)
    peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
  return 20.0 * log10 ((double)peak);
}

int
main (void)
{
  const size_t sizes[] = { 31, 63, 127 };
  const double freqs[] = { 3000.0, 4000.0, 5000.0, 6000.0 };
  printf ("taps ");
  for (size_t j = 0; j < 4; j++)
    printf ("  %4.0f Hz", freqs[j]);
  printf ("\n");
  for (size_t i = 0; i < 3; i++)
    {
      float h[MAX_TAPS];
      design (h, sizes[i], 4000.0);
      fir_t *f = fir_create (h, sizes[i]);
      if (f == NULL)
        return 1;
      printf ("%4zu ", sizes[i]);
      for (size_t j = 0; j < 4; j++)
        printf (" %8.1f", gain_db (f, sizes[i], freqs[j]));
      printf ("\n");
      fir_destroy (f);
    }
  printf ("(gain in dB)\n");
  return 0;
}
