/* chain.c -- a processor made of processors: COMPOSITION.
 *
 * A chain HAS stages, where a ramp IS a gain. It holds them only through
 * the interface, so a stage may be any kind, another chain included, and
 * the chain never needs to know which. It owns them: its destroy
 * destroys each, then itself.
 */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t   base;
  processor_t **stages; /* owned, and so is each stage */
  size_t        n;
} chain_t;

static void
chain_process (processor_t *self, float *buf, size_t n)
{
  chain_t *c = (chain_t *)self;
  for (size_t s = 0; s < c->n; s++)
    processor_process (c->stages[s], buf, n); /* whatever kind it is */
}

static void
chain_reset (processor_t *self)
{
  chain_t *c = (chain_t *)self;
  for (size_t s = 0; s < c->n; s++)
    processor_reset (c->stages[s]);
}

static void
chain_destroy (processor_t *self)
{
  chain_t *c = (chain_t *)self;
  for (size_t s = 0; s < c->n; s++)
    processor_destroy (c->stages[s]);
  free (c->stages);
  free (c);
}

static const processor_ops chain_ops = {
  .name    = "chain",
  .process = chain_process,
  .reset   = chain_reset,
  .destroy = chain_destroy,
};

processor_t *
chain_create (processor_t *const *stages, size_t n)
{
  chain_t *c = malloc (sizeof *c);
  if (c != NULL)
    c->stages = malloc (n * sizeof *c->stages);
  int ok = c != NULL && c->stages != NULL;
  for (size_t s = 0; s < n; s++)
    ok = ok && stages[s] != NULL;
  if (!ok) /* all or nothing: destroy every stage we were given */
    {
      for (size_t s = 0; s < n; s++)
        processor_destroy (stages[s]);
      if (c != NULL)
        free (c->stages);
      free (c);
      return NULL;
    }
  c->base.ops = &chain_ops;
  for (size_t s = 0; s < n; s++)
    c->stages[s] = stages[s];
  c->n = n;
  return &c->base;
}
