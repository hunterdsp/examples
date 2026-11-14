/* gain.c -- a processor that scales every sample, open to extension. */
#include "gain.h"

#include <stdlib.h>

void
gain_process (processor_t *self, float *buf, size_t n)
{
  gain_t *gain = (gain_t *)self;
  for (size_t i = 0; i < n; i++)
    buf[i] *= gain->g;
}

void
gain_destroy (processor_t *self)
{
  free (self);
}

/* No .reset: a gain has nothing to forget (lesson 16, exercise 3). */
static const processor_ops gain_ops = {
  .name    = "gain",
  .process = gain_process,
  .destroy = gain_destroy,
};

void
gain_init (gain_t *gain, const processor_ops *ops, float g)
{
  gain->base.ops = ops;
  gain->g        = g;
}

processor_t *
gain_create (float g)
{
  gain_t *gain = malloc (sizeof *gain);
  if (gain == NULL)
    return NULL;
  gain_init (gain, &gain_ops, g);
  return &gain->base;
}
