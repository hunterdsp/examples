/* ramp.c -- lesson 17's ramp, with its gain moved to the END.
 *
 * Now a ramp_t * and the gain_t * inside it are different addresses, so
 * a plain cast from one to the other is wrong. container_of gets from a
 * member back to the struct around it, by subtracting the member's
 * offset. And gain_destroy can't be inherited any more: it would free
 * the gain's address, which is not the start of the block malloc gave.
 */
#include "gain.h"

#include <stddef.h> /* offsetof */
#include <stdlib.h>

/* The struct of type `type` whose member `member` is at ptr. offsetof
   gives the member's distance from the start of the struct, in bytes;
   the char * makes the subtraction count bytes. */
#define container_of(ptr, type, member)                                       \
  ((type *)(void *)((char *)(ptr) - offsetof (type, member)))

typedef struct
{
  float  target; /* where g is gliding to */
  float  step;   /* how far g moves per sample */
  size_t left;   /* samples of glide still to go */
  gain_t gain;   /* the parent, LAST: no longer at the ramp's address */
} ramp_t;

/* Overrides gain_process: glide g sample by sample while a ramp is
   running, then hand the rest of the block to the parent's code. */
static void
ramp_process (processor_t *self, float *buf, size_t n)
{
  ramp_t *r = container_of (self, ramp_t, gain.base);
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
  ramp_t *r = container_of (self, ramp_t, gain.base);
  r->gain.g = r->target;
  r->left   = 0;
}

/* Overridden now: free the whole ramp, from the address malloc gave. */
static void
ramp_destroy (processor_t *self)
{
  free (container_of (self, ramp_t, gain.base));
}

static const processor_ops ramp_ops = {
  .name    = "ramp",
  .process = ramp_process, /* overridden */
  .reset   = ramp_reset,   /* added */
  .destroy = ramp_destroy, /* gain_destroy would free the wrong address */
};

int
ramp_set (processor_t *p, float g, size_t samples)
{
  if (p->ops != &ramp_ops) /* the one check that makes the cast safe */
    return -1;
  ramp_t *r = container_of (p, ramp_t, gain.base);
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
