/* gain.c -- a processor that scales every sample. */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t base; /* FIRST, so a gain_t * is also a processor_t * */
  float       g;
} gain_t;

static void
gain_process (processor_t *self, float *buf, size_t n)
{
  gain_t *gain = (gain_t *)self; /* back from the interface to the kind */
  for (size_t i = 0; i < n; i++)
    buf[i] *= gain->g;
}

static void
gain_reset (processor_t *self)
{
  (void)self; /* a gain remembers nothing, so there is nothing to reset */
}

static void
gain_destroy (processor_t *self)
{
  free (self); /* the address of base is the address of the whole gain_t */
}

/* One table for every gain there will ever be: static, const, shared. */
static const processor_ops gain_ops = {
  .name    = "gain",
  .process = gain_process,
  .reset   = gain_reset,
  .destroy = gain_destroy,
};

processor_t *
gain_create (float g)
{
  gain_t *gain = malloc (sizeof *gain);
  if (gain == NULL)
    return NULL;
  gain->base.ops = &gain_ops;
  gain->g        = g;
  return &gain->base;
}
