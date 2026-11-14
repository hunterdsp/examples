/* biquad.c -- design and run biquad filters (lesson 21). */
#include <sig/biquad.h>

#include "internal.h"

#include <math.h>
#include <stdlib.h>

struct sig_biquad
{
  float b0, b1, b2, a1, a2; /* what the filter is */
  float z1, z2;             /* what it remembers */
};

sig_status
sig_biquad_create (sig_biquad_type type, double f0_hz, double q,
                   double rate_hz, sig_biquad **out)
{
  *out = NULL;
  if (!sig__freq_ok (f0_hz, rate_hz) || !(q > 0.0)
      || (type != SIG_LOWPASS && type != SIG_HIGHPASS))
    return SIG_EINVAL;
  sig_biquad *f = malloc (sizeof *f);
  if (f == NULL)
    return SIG_ENOMEM;

  /* The Audio EQ Cookbook's low-pass and high-pass differ only in b0 and
     b1 (b2 equals b0 in both). Every coefficient is divided by a0, so
     the a0 of the textbook's equation is 1 and drops out of it. */
  double w0    = 2.0 * SIG_PI * f0_hz / rate_hz;
  double c     = cos (w0);
  double alpha = sin (w0) / (2.0 * q);
  double a0    = 1.0 + alpha;
  double b0    = type == SIG_LOWPASS ? (1.0 - c) / 2.0 : (1.0 + c) / 2.0;
  double b1    = type == SIG_LOWPASS ? 1.0 - c : -(1.0 + c);
  f->b0        = (float)(b0 / a0);
  f->b1        = (float)(b1 / a0);
  f->b2        = (float)(b0 / a0);
  f->a1        = (float)(-2.0 * c / a0);
  f->a2        = (float)((1.0 - alpha) / a0);
  f->z1        = 0.0f;
  f->z2        = 0.0f;
  *out         = f;
  return SIG_OK;
}

void
sig_biquad_destroy (sig_biquad *f)
{
  free (f);
}

void
sig_biquad_process (sig_biquad *f, const float *in, float *out, size_t n)
{
  float z1 = f->z1, z2 = f->z2;
  for (size_t i = 0; i < n; i++)
    {
      /* in[i] is read before out[i] is written, so in == out is safe. */
      float x = in[i];
      float y = f->b0 * x + z1;
      z1      = f->b1 * x - f->a1 * y + z2;
      z2      = f->b2 * x - f->a2 * y;
      out[i]  = y;
    }
  f->z1 = z1;
  f->z2 = z2;
}

void
sig_biquad_reset (sig_biquad *f)
{
  f->z1 = 0.0f;
  f->z2 = 0.0f;
}
