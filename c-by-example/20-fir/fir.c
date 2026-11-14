/* fir.c -- the FIR filter, in one allocation. */
#include "fir.h"

#include <stdlib.h>
#include <string.h>

struct fir
{
  size_t n;      /* number of taps */
  size_t newest; /* where in hist the latest input is */
  float *hist;   /* the last n inputs: a ring (lesson 12) */
  float  taps[]; /* n taps, then the n history slots: see fir_create */
};

fir_t *
fir_create (const float *taps, size_t n)
{
  /* taps[] is a FLEXIBLE ARRAY MEMBER (C99): the struct ends in an array
     of no fixed size, and one malloc makes room for the struct plus as
     many floats as asked for, here 2n: n taps and n history slots. */
  fir_t *f = malloc (sizeof *f + (size_t)2 * n * sizeof f->taps[0]);
  if (f == NULL)
    return NULL;
  f->n    = n;
  f->hist = f->taps + n;
  memcpy (f->taps, taps, n * sizeof taps[0]);
  fir_reset (f);
  return f;
}

void
fir_destroy (fir_t *f)
{
  free (f); /* one malloc, so one free */
}

void
fir_reset (fir_t *f)
{
  for (size_t k = 0; k < f->n; k++)
    f->hist[k] = 0.0f;
  f->newest = 0;
}

void
fir_process (fir_t *f, const float *in, float *out, size_t n)
{
  for (size_t i = 0; i < n; i++)
    {
      /* Move on one slot, overwriting the oldest input with the newest. */
      f->newest          = f->newest + 1 < f->n ? f->newest + 1 : 0;
      f->hist[f->newest] = in[i];

      /* Tap k meets the input from k samples ago, k slots back. */
      float  acc = 0.0f;
      size_t j   = f->newest;
      for (size_t k = 0; k < f->n; k++)
        {
          acc += f->taps[k] * f->hist[j];
          j = j > 0 ? j - 1 : f->n - 1;
        }
      out[i] = acc;
    }
}
