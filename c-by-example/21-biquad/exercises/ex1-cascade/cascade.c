/* cascade.c -- two biquads in a row: a fourth-order filter.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       cascade.c ../../biquad.c -o cascade -lm && ./cascade
 *
 * One biquad falls by 12 dB per octave past its corner. Two in series
 * fall by 24, and with the right two q's, 0.5412 and 1.3066, the pair is
 * a fourth-order Butterworth filter: still flat below the corner, still
 * -3 dB at it, and twice as steep after it.
 */
#include "biquad.h"

#include <math.h>
#include <stdio.h>

enum
{
  LEN  = 9600,
  SKIP = 4800
};

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

/* Gain in dB through `count` biquads in series. */
static double
gain_db (const biquad_coeffs *c, size_t count, double freq_hz)
{
  static float x[LEN];
  for (size_t i = 0; i < LEN; i++)
    x[i] = (float)sin (2.0 * PI * freq_hz * (double)i / RATE);
  for (size_t s = 0; s < count; s++)
    {
      biquad_t f = { .c = c[s] };
      biquad_process (&f, x, x, LEN); /* each stage filters the last */
    }
  float peak = 0.0f;
  for (size_t i = SKIP; i < LEN; i++)
    peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
  return 20.0 * log10 ((double)peak);
}

int
main (void)
{
  const biquad_coeffs one[1]  = { biquad_lowpass (1000.0, 0.7071, RATE) };
  const biquad_coeffs four[2] = { biquad_lowpass (1000.0, 0.5412, RATE),
                                  biquad_lowpass (1000.0, 1.3066, RATE) };
  const double        freqs[] = { 500.0, 1000.0, 2000.0, 4000.0 };
  printf ("          one biquad   two biquads\n");
  for (size_t i = 0; i < sizeof freqs / sizeof freqs[0]; i++)
    printf ("%5.0f Hz: %8.2f dB  %8.2f dB\n", freqs[i],
            gain_db (one, 1, freqs[i]), gain_db (four, 2, freqs[i]));
  return 0;
}
