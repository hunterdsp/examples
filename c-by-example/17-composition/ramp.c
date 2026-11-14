/* ramp.c -- a gain that glides: a kind DERIVED from gain.
 *
 * A ramp_t starts with a whole gain_t, which starts with a processor_t,
 * so one address is all three at once. A ramp IS a gain: anything that
 * works on a gain_t works on a ramp. It adds a target and a step, and
 * of the gain's operations it keeps one (destroy), replaces one
 * (process) and adds one (reset).
 */
#include "gain.h"

#include <stdlib.h>

typedef struct
{
  gain_t gain;   /* first: the PARENT, so a ramp_t * is a gain_t * */
  float  target; /* where g is gliding to */
  float  step;   /* how far g moves per sample */
  size_t left;   /* samples of glide still to go */
} ramp_t;

/* Overrides gain_process: glide g sample by sample while a ramp is
   running, then hand the rest of the block to the parent's code. */
static void
ramp_process (processor_t *self, float *buf, size_t n)
{
  ramp_t *r = (ramp_t *)self;
  size_t  i = 0;
  for (; i < n && r->left > 0; i++)
    {
      r->left--;
      r->gain.g = r->left > 0 ? r->gain.g + r->step : r->target;
      buf[i] *= r->gain.g;
    }
  gain_process (self, buf + i, n - i); /* the parent does the steady part */
}

/* Added: a gain has nothing to reset, but a ramp jumps to its target. */
static void
ramp_reset (processor_t *self)
{
  ramp_t *r = (ramp_t *)self;
  r->gain.g = r->target;
  r->left   = 0;
}

static const processor_ops ramp_ops = {
  .name    = "ramp",
  .process = ramp_process, /* overridden */
  .reset   = ramp_reset,   /* added */
  .destroy = gain_destroy, /* inherited: a ramp owns nothing more */
};

int
ramp_set (processor_t *p, float g, size_t samples)
{
  if (p->ops != &ramp_ops) /* the one check that makes the cast safe */
    return -1;
  ramp_t *r = (ramp_t *)p;
  r->target = g;
  r->left   = samples;
  r->step   = samples > 0 ? (g - r->gain.g) / (float)samples : 0.0f;
  if (samples == 0)
    r->gain.g = g;
  return 0;
}

processor_t *
ramp_create (float g, size_t samples)
{
  ramp_t *r = malloc (sizeof *r);
  if (r == NULL)
    return NULL;
  gain_init (&r->gain, &ramp_ops, 0.0f); /* the parent's setup first */
  ramp_set (&r->gain.base, g, samples);
  return &r->gain.base;
}
