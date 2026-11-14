/* fade.c -- a fade-in for ANY processor: composition, not inheritance.
 *
 * The ramp IS a gain, so it can only ever fade a gain. A fade HAS a
 * processor, any kind, reached only through the interface: it runs it,
 * then scales what comes out. It needs nothing from gain.h, and it
 * works on a low-pass, a chain, or a fade.
 */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t  base;
  processor_t *inner;   /* owned */
  size_t       samples; /* length of the fade */
  size_t       done;    /* samples faded so far */
} fade_t;

static void
fade_process (processor_t *self, float *buf, size_t n)
{
  fade_t *f = (fade_t *)self;
  processor_process (f->inner, buf, n); /* whatever it is, it runs */
  for (size_t i = 0; i < n && f->done < f->samples; i++)
    {
      f->done++;
      buf[i] *= (float)f->done / (float)f->samples;
    }
}

static void
fade_reset (processor_t *self)
{
  fade_t *f = (fade_t *)self;
  processor_reset (f->inner);
  f->done = 0; /* and fade in again */
}

static void
fade_destroy (processor_t *self)
{
  fade_t *f = (fade_t *)self;
  processor_destroy (f->inner);
  free (f);
}

static const processor_ops fade_ops = {
  .name    = "fade",
  .process = fade_process,
  .reset   = fade_reset,
  .destroy = fade_destroy,
};

processor_t *
fade_create (processor_t *inner, size_t samples)
{
  fade_t *f = malloc (sizeof *f);
  if (f == NULL || inner == NULL)
    {
      processor_destroy (inner);
      free (f);
      return NULL;
    }
  f->base.ops = &fade_ops;
  f->inner    = inner;
  f->samples  = samples;
  f->done     = 0;
  return &f->base;
}
