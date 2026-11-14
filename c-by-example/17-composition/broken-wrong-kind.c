/* broken-wrong-kind.c -- treating a gain as a ramp.
 *
 * A ramp_t is a gain_t with more after it. Casting a ramp to a gain is
 * always safe: the gain is its first part. Casting a gain to a ramp is
 * not: a plain gain has no `target` after it, so writing one lands past
 * the end of its block, and AddressSanitizer stops the program. ramp.c's
 * ramp_set checks the table first for exactly this reason; this one
 * doesn't.
 */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t base;
  float       g;
} gain_t;

typedef struct
{
  gain_t gain;
  float  target; /* past the end of a gain_t */
} ramp_t;

static void
gain_destroy (processor_t *self)
{
  free (self);
}

static void
gain_process (processor_t *self, float *buf, size_t n)
{
  for (size_t i = 0; i < n; i++)
    buf[i] *= ((gain_t *)self)->g;
}

static const processor_ops gain_ops = {
  .name    = "gain",
  .process = gain_process,
  .destroy = gain_destroy,
};

/* No check of p->ops: it trusts the caller to pass a ramp. */
static void
ramp_set_target (processor_t *p, float target)
{
  ((ramp_t *)p)->target = target;
}

int
main (void)
{
  gain_t *gain = malloc (sizeof *gain);
  if (gain == NULL)
    return 1;
  gain->base.ops = &gain_ops;
  gain->g        = 1.0f;
  ramp_set_target (&gain->base, 0.5f); /* a gain, not a ramp */
  processor_destroy (&gain->base);
  return 0;
}
