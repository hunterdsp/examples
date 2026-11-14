/* lowpass_double.c -- lowpass.c with its memory kept in double.
 *
 * Same header, same functions, a different struct: twice the size, and
 * the running output no longer rounded to float between samples.
 */
#include "lowpass.h"

#include <stdlib.h>

/* The definition the header leaves out. Change it, and only this file
   needs recompiling: no caller ever saw it. */
struct lowpass
{
  double a; /* smoothing coefficient, 0 < a <= 1 */
  double y; /* the last output, now kept in double */
};

lowpass_t *
lowpass_create (float a)
{
  if (!(a > 0.0f && a <= 1.0f)) /* written so a NaN fails it too */
    return NULL;
  lowpass_t *f = malloc (sizeof *f);
  if (f == NULL)
    return NULL;
  f->a = (double)a;
  f->y = 0.0;
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
  const double a = f->a; /* local copies: the loop needn't touch *f */
  double       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * ((double)buf[i] - y);
      buf[i] = (float)y;
    }
  f->y = y; /* remembered for the next block */
}

void
lowpass_reset (lowpass_t *f)
{
  f->y = 0.0;
}
