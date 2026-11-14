/* mix.c -- two processors side by side: parallel composition.
 *
 * A chain runs its stages one after another. A mix runs both on the
 * same input and adds the results, the way a "dry/wet" control blends
 * the plain signal with the effect. b needs its own copy of the input,
 * so the mix keeps a scratch buffer, allocated once, at create (lesson
 * 13), and processes a long block in pieces that fit it.
 */
#include "processor.h"

#include <stdlib.h>
#include <string.h>

enum
{
  SCRATCH = 256 /* samples: one audio block */
};

typedef struct
{
  processor_t  base;
  processor_t *a, *b; /* owned */
  float        scratch[SCRATCH];
} mix_t;

static void
mix_process (processor_t *self, float *buf, size_t n)
{
  mix_t *m = (mix_t *)self;
  for (size_t start = 0; start < n; start += SCRATCH)
    {
      float *part = buf + start;
      size_t len  = n - start < SCRATCH ? n - start : SCRATCH;
      memcpy (m->scratch, part, len * sizeof *part); /* b's copy */
      processor_process (m->a, part, len);
      processor_process (m->b, m->scratch, len);
      for (size_t i = 0; i < len; i++)
        part[i] += m->scratch[i];
    }
}

static void
mix_reset (processor_t *self)
{
  mix_t *m = (mix_t *)self;
  processor_reset (m->a);
  processor_reset (m->b);
}

static void
mix_destroy (processor_t *self)
{
  mix_t *m = (mix_t *)self;
  processor_destroy (m->a);
  processor_destroy (m->b);
  free (m);
}

static const processor_ops mix_ops = {
  .name    = "mix",
  .process = mix_process,
  .reset   = mix_reset,
  .destroy = mix_destroy,
};

processor_t *
mix_create (processor_t *a, processor_t *b)
{
  mix_t *m = malloc (sizeof *m);
  if (m == NULL || a == NULL || b == NULL)
    {
      processor_destroy (a); /* all or nothing, like chain_create */
      processor_destroy (b);
      free (m);
      return NULL;
    }
  m->base.ops = &mix_ops;
  m->a        = a;
  m->b        = b;
  return &m->base;
}
