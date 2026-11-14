/* gain.h -- the inside of a gain, for kinds that EXTEND it.
 *
 * Callers that only use a gain need processor.h and nothing else. A kind
 * that builds on a gain (ramp.c) needs more: the struct's layout, to put
 * a gain_t first in its own struct, and the gain's functions, to reuse
 * them. That is what this header gives, and the price is that gain_t is
 * no longer private: change it, and every kind built on it recompiles.
 */
#ifndef GAIN_H
#define GAIN_H

#include "processor.h"

typedef struct
{
  processor_t base; /* first: a gain_t * is a processor_t * */
  float       g;
} gain_t;

/* Set up a gain in memory the caller has, with the given table: a kind
   built on gain passes its own. */
void gain_init (gain_t *gain, const processor_ops *ops, float g);

/* The gain's operations, for a derived kind to reuse or call. */
void gain_process (processor_t *self, float *buf, size_t n);
void gain_destroy (processor_t *self);

#endif /* GAIN_H */
