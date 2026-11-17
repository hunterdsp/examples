/* broken-ring-overflow.c -- a ring that forgot to wrap.
 *
 * The index is the push count itself, with no `& MASK`, and nothing
 * checks for full. The first 8 pushes land in buf[0..7]; the 9th writes
 * buf[8], past the end of the array, onto whatever the struct keeps next.
 * UndefinedBehaviorSanitizer knows the array has 8 elements, and
 * reports the bad index.
 */
#include <stddef.h>
#include <stdio.h>

enum
{
  CAP = 8
};

typedef struct
{
  float  buf[CAP];
  size_t head;
} ring_t;

static void
ring_push (ring_t *r, float x)
{
  r->buf[r->head] = x; /* should be r->buf[r->head & MASK] */
  r->head++;
}

int
main (void)
{
  ring_t r = { { 0 }, 0 };
  for (int i = 1; i <= 10; i++)
    ring_push (&r, (float)i);
  printf ("pushed %zu\n", r.head);
  return 0;
}
