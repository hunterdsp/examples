/* average.c -- lesson 04's moving average, as an FIR filter.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       average.c ../../fir.c -o average -lm && ./average
 *
 * Averaging the last 8 samples is an FIR filter whose 8 taps are all
 * 1/8. Measuring it like any other filter shows what it does to each
 * frequency, which the word "average" doesn't.
 */
#include "fir.h"

#include <math.h>
#include <stdio.h>

enum
{
  TAPS = 8,
  LEN  = 4800
};

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

static double
gain_db (fir_t *f, double freq_hz)
{
  static float x[LEN];
  for (size_t i = 0; i < LEN; i++)
    x[i] = (float)sin (2.0 * PI * freq_hz * (double)i / RATE);
  fir_reset (f);
  fir_process (f, x, x, LEN);
  float peak = 0.0f;
  for (size_t i = TAPS; i < LEN; i++)
    peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
  return 20.0 * log10 ((double)peak);
}

int
main (void)
{
  float h[TAPS];
  for (size_t k = 0; k < TAPS; k++)
    h[k] = 1.0f / (float)TAPS;
  fir_t *f = fir_create (h, TAPS);
  if (f == NULL)
    return 1;

  const double freqs[] = { 1000.0, 3000.0, 6000.0, 9000.0, 12000.0 };
  for (size_t i = 0; i < sizeof freqs / sizeof freqs[0]; i++)
    {
      double db = gain_db (f, freqs[i]);
      if (db < -80.0) /* rounding noise, not signal */
        printf ("%6.0f Hz: nothing gets through\n", freqs[i]);
      else
        printf ("%6.0f Hz: %6.1f dB\n", freqs[i], db);
    }
  fir_destroy (f);
  return 0;
}
