/* delay.c -- a third kind of processor: every sample, DELAY samples late.
 *
 * Its memory is the last DELAY inputs, kept in a small ring (lesson 12).
 * Unlike a gain it has something to forget, so its reset matters.
 */
#include "processor.h"

#include <stdlib.h>

enum
{
  DELAY = 3
};

typedef struct
{
  processor_t base;
  float       past[DELAY]; /* the inputs still to come out */
  size_t      next;        /* the oldest of them, next to come out */
} delay_t;

static void
delay_process (processor_t *self, float *buf, size_t n)
{
  delay_t *d = (delay_t *)self;
  for (size_t i = 0; i < n; i++)
    {
      float out        = d->past[d->next];
      d->past[d->next] = buf[i];
      d->next          = (d->next + (size_t)1) % (size_t)DELAY;
      buf[i]           = out;
    }
}

static void
delay_reset (processor_t *self)
{
  delay_t *d = (delay_t *)self;
  for (size_t i = 0; i < DELAY; i++)
    d->past[i] = 0.0f;
  d->next = 0;
}

static void
delay_destroy (processor_t *self)
{
  free (self);
}

static const processor_ops delay_ops = {
  .name    = "delay",
  .process = delay_process,
  .reset   = delay_reset,
  .destroy = delay_destroy,
};

processor_t *
delay_create (void)
{
  delay_t *d = malloc (sizeof *d);
  if (d == NULL)
    return NULL;
  d->base.ops = &delay_ops;
  delay_reset (&d->base); /* a new delay starts silent */
  return &d->base;
}
