/* lowpass.c -- design a low-pass FIR filter, then measure what it does.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       lowpass.c fir.c -o lowpass -lm && ./lowpass
 *
 * The taps are a WINDOWED SINC: the ideal low-pass filter's impulse
 * response (a sinc), cut to 31 taps and tapered at the ends by a Hamming
 * window so the cut doesn't ring. The program then checks the filter the
 * way an engineer would: feed it an impulse, a step, and sine waves, and
 * look at what comes out.
 */
#include "fir.h"

#include <math.h>
#include <stdio.h>

enum
{
  TAPS = 31,
  LEN  = 4800 /* 0.1 s at 48 kHz */
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
   the output's peak, once the first TAPS samples have passed, with 1. */
static double
gain_db (fir_t *f, double freq_hz)
{
  static float x[LEN];
  for (size_t i = 0; i < LEN; i++)
    x[i] = (float)sin (2.0 * PI * freq_hz * (double)i / RATE);
  fir_reset (f);
  fir_process (f, x, x, LEN); /* in place */
  float peak = 0.0f;
  for (size_t i = TAPS; i < LEN; i++)
    peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
  return 20.0 * log10 ((double)peak);
}

int
main (void)
{
  float h[TAPS];
  design (h, TAPS, 4000.0);
  fir_t *f = fir_create (h, TAPS);
  if (f == NULL)
    return 1;

  /* An impulse in: the taps come out, one per sample. */
  float x[TAPS] = { 1.0f }, y[TAPS];
  fir_process (f, x, y, TAPS);
  int same = 1;
  for (size_t k = 0; k < TAPS; k++)
    same = same && y[k] == h[k];
  printf ("impulse response is the taps: %s\n", same ? "yes" : "no");

  /* A step in: the output settles at the sum of the taps, the DC gain. */
  float step[2 * TAPS];
  for (size_t i = 0; i < 2 * TAPS; i++)
    step[i] = 1.0f;
  fir_reset (f);
  fir_process (f, step, step, 2 * TAPS);
  printf ("step settles at %.4f\n", (double)step[2 * TAPS - 1]);

  /* Sines in: how much of each frequency gets through. */
  const double freqs[] = { 1000.0, 3000.0, 4000.0, 6000.0, 12000.0 };
  for (size_t i = 0; i < sizeof freqs / sizeof freqs[0]; i++)
    printf ("%6.0f Hz: %7.2f dB\n", freqs[i], gain_db (f, freqs[i]));

  fir_destroy (f);
  return 0;
}
