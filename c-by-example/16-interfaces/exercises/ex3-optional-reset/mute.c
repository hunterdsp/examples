/* mute.c -- the mute from broken-missing-op.c, now that reset is optional.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       mute.c -o mute && ./mute
 *
 * mute_ops still names no .reset. This processor.h documents that reset
 * may be NULL, and processor_reset checks before calling.
 */
#include "processor.h"

#include <stdio.h>
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
  .destroy = mute_destroy, /* no .reset: allowed now */
};

int
main (void)
{
  mute_t *m = malloc (sizeof *m);
  if (m == NULL)
    return 1;
  m->base.ops = &mute_ops;

  float buf[4] = { 1.0f, -1.0f, 0.5f, -0.5f };
  processor_process (&m->base, buf, 4);
  printf ("%s:", processor_name (&m->base));
  for (size_t i = 0; i < 4; i++)
    printf (" %.1f", (double)buf[i]);
  printf ("\n");

  processor_reset (&m->base); /* skipped: mute has no reset */
  printf ("reset: nothing to do, and no crash\n");
  processor_destroy (&m->base);
  return 0;
}
