/* tables.c -- one table per kind, one pointer per object.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       tables.c gain.c lowpass.c -o tables && ./tables
 *
 * Every gain points at the same gain_ops, so a table costs its memory
 * once per KIND, and each object pays one pointer for it. The other
 * design, every object carrying its own function pointers, is below for
 * comparison.
 */
#include "processor.h"

#include <stdio.h>

/* The alternative: the functions in every object, not in a shared table. */
typedef struct fat fat_t;
struct fat
{
  const char *name;
  void (*process) (fat_t *self, float *buf, size_t n);
  void (*reset) (fat_t *self);
  void (*destroy) (fat_t *self);
};

int
main (void)
{
  processor_t *a = gain_create (0.5f);
  processor_t *b = gain_create (2.0f);
  processor_t *f = lowpass_create (0.5f);
  if (a != NULL && b != NULL && f != NULL)
    {
      printf ("two gains, one table:     %s\n",
              a->ops == b->ops ? "yes" : "no");
      printf ("gain and lowpass share:   %s\n",
              a->ops == f->ops ? "yes" : "no");
      printf ("interface, per object:    %zu bytes\n", sizeof (processor_t));
      printf ("own pointers, per object: %zu bytes\n", sizeof (fat_t));
    }
  processor_destroy (a);
  processor_destroy (b);
  processor_destroy (f);
  return 0;
}
