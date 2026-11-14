/* lowpass.c -- lesson 14's one-pole low-pass, as a processor. */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t base; /* first, as in every kind */
  float       a;    /* smoothing coefficient, 0 < a <= 1 */
  float       y;    /* the last output */
} lowpass_t;

static void
lowpass_process (processor_t *self, float *buf, size_t n)
{
  lowpass_t  *f = (lowpass_t *)self;
  const float a = f->a;
  float       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * (buf[i] - y);
      buf[i] = y;
    }
  f->y = y;
}

static void
lowpass_reset (processor_t *self)
{
  ((lowpass_t *)self)->y = 0.0f;
}

static void
lowpass_destroy (processor_t *self)
{
  free (self);
}

static const processor_ops lowpass_ops = {
  .name    = "lowpass",
  .process = lowpass_process,
  .reset   = lowpass_reset,
  .destroy = lowpass_destroy,
};

processor_t *
lowpass_create (float a)
{
  if (!(a > 0.0f && a <= 1.0f))
    return NULL;
  lowpass_t *f = malloc (sizeof *f);
  if (f == NULL)
    return NULL;
  f->base.ops = &lowpass_ops;
  f->a        = a;
  f->y        = 0.0f;
  return &f->base;
}
