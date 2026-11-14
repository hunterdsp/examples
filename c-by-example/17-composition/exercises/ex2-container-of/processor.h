/* processor.h -- lesson 16's interface, with reset optional (lesson 16,
 * exercise 3), and three kinds more.
 *
 * A processor is a pointer to its operations, nothing more:
 *
 *   struct processor { const processor_ops *ops; };
 *
 * Each kind of processor (a gain, a low-pass, ...) puts a processor_t as
 * the FIRST member of its own struct and fills in one processor_ops: its
 * VTABLE, the table of functions that do the work for that kind. Code
 * that holds a processor_t * calls through the table, and never needs to
 * know which kind it has.
 */
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stddef.h>

typedef struct processor     processor_t;
typedef struct processor_ops processor_ops;

/* What every processor can do. Each function takes the processor itself
   first, as `self`: the data the function works on. */
struct processor_ops
{
  const char *name;
  void (*process) (processor_t *self, float *buf, size_t n);
  void (*reset) (processor_t *self); /* may be NULL: nothing to reset */
  void (*destroy) (processor_t *self);
};

struct processor
{
  const processor_ops *ops; /* one shared table per kind */
};

/* Calls through the table, so callers needn't spell out p->ops->... */
static inline void
processor_process (processor_t *p, float *buf, size_t n)
{
  p->ops->process (p, buf, n);
}

static inline void
processor_reset (processor_t *p)
{
  if (p->ops->reset != NULL)
    p->ops->reset (p);
}

static inline void
processor_destroy (processor_t *p)
{
  if (p != NULL)
    p->ops->destroy (p);
}

static inline const char *
processor_name (const processor_t *p)
{
  return p->ops->name;
}

/* The kinds this lesson has. Each returns a new processor, owned by the
   caller and freed with processor_destroy, or NULL. */
processor_t *gain_create (float g);
processor_t *lowpass_create (float a);

/* A gain that glides from 0 to g over `samples` samples: a fade-in. */
processor_t *ramp_create (float g, size_t samples);

/* Glide p, which must be a ramp, to g over `samples` samples. Returns 0,
   or -1 (and does nothing) if p is some other kind. */
int ramp_set (processor_t *p, float g, size_t samples);

/* A processor made of n others, run in order. The chain takes ownership
   of the stages: destroying it destroys them. If it can't be created, it
   destroys them anyway, so the caller never has to. */
processor_t *chain_create (processor_t *const *stages, size_t n);

#endif /* PROCESSOR_H */
