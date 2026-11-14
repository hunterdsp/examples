/* lowpass.c -- the only file that knows what a lowpass_t holds. */
#include "lowpass.h"

#include <stdlib.h>

/* The definition the header leaves out. Change it, and only this file
   needs recompiling: no caller ever saw it. */
struct lowpass
{
  float a; /* smoothing coefficient, 0 < a <= 1 */
  float y; /* the last output: the filter's memory */
};

lowpass_t *
lowpass_create (float a)
{
  if (!(a > 0.0f && a <= 1.0f)) /* written so a NaN fails it too */
    return NULL;
  lowpass_t *f = malloc (sizeof *f);
  if (f == NULL)
    return NULL;
  f->a = a;
  f->y = 0.0f;
  return f;
}

void
lowpass_destroy (lowpass_t *f)
{
  free (f); /* free(NULL) does nothing, so neither does this */
}

void
lowpass_process (lowpass_t *f, float *buf, size_t n)
{
  const float a = f->a; /* local copies: the loop needn't touch *f */
  float       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * (buf[i] - y);
      buf[i] = y;
    }
  f->y = y; /* remembered for the next block */
}

void
lowpass_reset (lowpass_t *f)
{
  f->y = 0.0f;
}
