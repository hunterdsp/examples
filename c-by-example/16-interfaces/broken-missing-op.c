/* broken-missing-op.c -- a vtable with a hole in it.
 *
 * A designated initializer sets the members it names and leaves the rest
 * zero. mute_ops names no .reset, so its reset is a NULL pointer, and
 * the first processor_reset on a mute calls address 0. Nothing checked
 * when the table was written; the crash comes when the hole is used.
 */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t base;
} mute_t;

static void
mute_process (processor_t *self, float *buf, size_t n)
{
  (void)self;
  for (size_t i = 0; i < n; i++)
    buf[i] = 0.0f;
}

static void
mute_destroy (processor_t *self)
{
  free (self);
}

static const processor_ops mute_ops = {
  .name    = "mute",
  .process = mute_process,
  .destroy = mute_destroy, /* no .reset: it is NULL */
};

int
main (void)
{
  mute_t *m = malloc (sizeof *m);
  if (m == NULL)
    return 1;
  m->base.ops = &mute_ops;
  processor_reset (&m->base); /* calls a NULL function pointer */
  processor_destroy (&m->base);
  return 0;
}
