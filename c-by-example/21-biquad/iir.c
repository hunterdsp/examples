/* iir.c -- a biquad low-pass, measured, and checked against the equation.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       iir.c biquad.c -o iir -lm && ./iir
 */
#include "biquad.h"

#include <math.h>
#include <stdio.h>

enum
{
  LEN  = 9600, /* 0.2 s at 48 kHz */
  SKIP = 4800  /* samples to let the filter settle before measuring */
};

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

static double
gain_db (biquad_coeffs c, double freq_hz)
{
  static float x[LEN];
  for (size_t i = 0; i < LEN; i++)
    x[i] = (float)sin (2.0 * PI * freq_hz * (double)i / RATE);
  biquad_t f = { .c = c }; /* state starts at zero */
  biquad_process (&f, x, x, LEN);
  float peak = 0.0f;
  for (size_t i = SKIP; i < LEN; i++)
    peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
  return 20.0 * log10 ((double)peak);
}

/* The equation in biquad.h, written out as it reads: DIRECT FORM I. It
   keeps four numbers of memory, the last two inputs and outputs. */
static void
direct_form_1 (biquad_coeffs c, const float *in, float *out, size_t n)
{
  float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
  for (size_t i = 0; i < n; i++)
    {
      float y = c.b0 * in[i] + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
      x2      = x1;
      x1      = in[i];
      y2      = y1;
      y1      = y;
      out[i]  = y;
    }
}

int
main (void)
{
  biquad_coeffs lp = biquad_lowpass (1000.0, 0.7071, RATE);
  printf ("b0 %.6f  b1 %.6f  b2 %.6f  a1 %.6f  a2 %.6f\n", (double)lp.b0,
          (double)lp.b1, (double)lp.b2, (double)lp.a1, (double)lp.a2);

  /* An impulse: the response dies away, but never reaches exactly 0. */
  static float imp[LEN], y[LEN], y1[LEN];
  imp[0]     = 1.0f;
  biquad_t f = { .c = lp };
  biquad_process (&f, imp, y, LEN);
  const size_t at[] = { 0, 1, 2, 10, 100 };
  printf ("impulse response:");
  for (size_t k = 0; k < sizeof at / sizeof at[0]; k++)
    printf (" [%zu] %.2e", at[k], (double)y[at[k]]);
  printf ("\n[1000] is below 1e-30: %s; exactly 0: %s\n",
          fabsf (y[1000]) < 1e-30f ? "yes" : "no",
          y[1000] == 0.0f ? "yes" : "no");

  /* Gain, an octave apart: past the corner, 12 dB less per octave. */
  const double freqs[] = { 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0 };
  for (size_t i = 0; i < sizeof freqs / sizeof freqs[0]; i++)
    printf ("%5.0f Hz: %7.2f dB\n", freqs[i], gain_db (lp, freqs[i]));

  /* The two forms compute the same filter. */
  direct_form_1 (lp, imp, y1, LEN);
  double worst = 0.0;
  for (size_t i = 0; i < LEN; i++)
    worst = fmax (worst, fabs ((double)y[i] - (double)y1[i]));
  printf ("direct form I agrees within 1e-6: %s\n",
          worst < 1e-6 ? "yes" : "no");
  return 0;
}
