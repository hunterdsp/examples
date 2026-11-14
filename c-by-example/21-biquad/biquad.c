/* biquad.c -- design and run biquad filters. */
#include "biquad.h"

#include <math.h>

static const double PI = 3.141592653589793;

/* The low-pass and high-pass designs differ only in their b's, so both
   start here. These are the formulas of Robert Bristow-Johnson's "Audio
   EQ Cookbook", the usual source: they put the corner exactly at f0_hz. */
static biquad_coeffs
design (double f0_hz, double q, double rate_hz, double b0, double b1)
{
  double w0    = 2.0 * PI * f0_hz / rate_hz;
  double alpha = sin (w0) / (2.0 * q);
  double a0    = 1.0 + alpha;
  /* Every coefficient is divided by a0, so the a0 of the textbook's
     equation is 1 and drops out of it. */
  biquad_coeffs c = {
    .b0 = (float)(b0 / a0),
    .b1 = (float)(b1 / a0),
    .b2 = (float)(b0 / a0), /* b2 equals b0 in both designs */
    .a1 = (float)(-2.0 * cos (w0) / a0),
    .a2 = (float)((1.0 - alpha) / a0),
  };
  return c;
}

biquad_coeffs
biquad_lowpass (double f0_hz, double q, double rate_hz)
{
  double c = cos (2.0 * PI * f0_hz / rate_hz);
  return design (f0_hz, q, rate_hz, (1.0 - c) / 2.0, 1.0 - c);
}

biquad_coeffs
biquad_highpass (double f0_hz, double q, double rate_hz)
{
  double c = cos (2.0 * PI * f0_hz / rate_hz);
  return design (f0_hz, q, rate_hz, (1.0 + c) / 2.0, -(1.0 + c));
}

void
biquad_process (biquad_t *f, const float *in, float *out, size_t n)
{
  const biquad_coeffs c  = f->c;
  float               z1 = f->z1, z2 = f->z2;
  for (size_t i = 0; i < n; i++)
    {
      /* Transposed direct form II: see the README for how these three
         lines come from the equation in biquad.h. */
      float x = in[i];
      float y = c.b0 * x + z1;
      z1      = c.b1 * x - c.a1 * y + z2;
      z2      = c.b2 * x - c.a2 * y;
      out[i]  = y;
    }
  f->z1 = z1;
  f->z2 = z2;
}
