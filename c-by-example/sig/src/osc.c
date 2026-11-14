/* osc.c -- a sine oscillator with a 32-bit phase accumulator.
 *
 * A whole cycle is the whole range of a uint32_t, and unsigned arithmetic
 * wraps at 2^32, so the phase wraps exactly where a cycle ends (lesson
 * 22). The top 10 bits pick an entry of a 1024-entry sine table, and the
 * low 22 bits say how far it is to the next one.
 */
#include <sig/osc.h>

#include "internal.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

enum
{
  TABLE_BITS = 10,
  TABLE      = 1 << TABLE_BITS, /* 1024 */
  FRAC_BITS  = 32 - TABLE_BITS  /* 22 */
};

static const double CYCLE = 4294967296.0; /* 2^32: one whole cycle */

struct sig_osc
{
  uint32_t phase;            /* where in the cycle, 0 .. 2^32 - 1 */
  uint32_t inc;              /* how far one sample moves it */
  float    table[TABLE + 1]; /* [TABLE] repeats [0], for k + 1 */
};

sig_status
sig_osc_create (double freq_hz, double rate_hz, sig_osc **out)
{
  *out = NULL;
  if (!sig__freq_ok (freq_hz, rate_hz))
    return SIG_EINVAL;
  sig_osc *o = malloc (sizeof *o);
  if (o == NULL)
    return SIG_ENOMEM;
  /* Each oscillator has its own table, so the library has no global
     state to set up, or to share between threads. */
  for (size_t k = 0; k <= TABLE; k++)
    o->table[k] = (float)sin (2.0 * SIG_PI * (double)k / (double)TABLE);
  o->phase = 0;
  /* freq_hz < rate_hz / 2, so inc < 2^31: it fits. */
  o->inc = (uint32_t)(freq_hz / rate_hz * CYCLE + 0.5);
  *out   = o;
  return SIG_OK;
}

void
sig_osc_destroy (sig_osc *o)
{
  free (o);
}

void
sig_osc_process (sig_osc *o, float *out, size_t n)
{
  const float *t     = o->table;
  uint32_t     phase = o->phase;
  for (size_t i = 0; i < n; i++)
    {
      uint32_t k    = phase >> FRAC_BITS;
      float    frac = (float)(phase & ((UINT32_C (1) << FRAC_BITS) - 1u))
                      / (float)(UINT32_C (1) << FRAC_BITS);
      out[i]        = t[k] + frac * (t[k + 1] - t[k]);
      phase += o->inc; /* wraps at 2^32: no if, no fmod */
    }
  o->phase = phase;
}
