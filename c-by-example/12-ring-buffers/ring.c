/* ring.c -- a ring buffer: a fixed array used as a queue that never ends.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       ring.c -o ring && ./ring
 *
 * Samples go in at one end (push) and come out at the other (pop), first
 * in, first out. Two counters say where: `head` counts every sample ever
 * pushed, `tail` every sample ever popped. They only ever go up; the
 * array index is the count modulo the capacity, so the writes go round
 * and round the same CAP slots. No sample is ever moved, and no memory
 * is ever allocated.
 */
#include <stddef.h>
#include <stdio.h>

/* A power of two, so `count % CAP` is `count & MASK`: one AND, no
   division (exercise 1). */
enum
{
  CAP  = 8,
  MASK = CAP - 1 /* 0b111: keeps the low 3 bits, 0..7 */
};

typedef struct
{
  float  buf[CAP];
  size_t head; /* samples pushed, ever: the next write is buf[head & MASK] */
  size_t tail; /* samples popped, ever: the next read is buf[tail & MASK] */
} ring_t;

/* How many samples are waiting. Right even after head wraps past
   SIZE_MAX, because unsigned subtraction is modulo 2^N (lesson 02). */
static size_t
ring_count (const ring_t *r)
{
  return r->head - r->tail;
}

/* Add x at the head. Returns 0, or -1 if the ring is full. */
static int
ring_push (ring_t *r, float x)
{
  if (ring_count (r) == (size_t)CAP)
    return -1;
  r->buf[r->head & (size_t)MASK] = x;
  r->head++;
  return 0;
}

/* Take the oldest sample into *x. Returns 0, or -1 if the ring is empty. */
static int
ring_pop (ring_t *r, float *x)
{
  if (ring_count (r) == (size_t)0)
    return -1;
  *x = r->buf[r->tail & (size_t)MASK];
  r->tail++;
  return 0;
}

int
main (void)
{
  ring_t r = { { 0 }, 0, 0 };

  /* Push 10 samples into 8 slots: the last two are refused. */
  int refused = 0;
  for (int i = 1; i <= 10; i++)
    if (ring_push (&r, (float)i) != 0)
      refused++;
  printf ("pushed 1..10 into %d slots: %zu waiting, %d refused (full)\n", CAP,
          ring_count (&r), refused);

  float x;
  printf ("pop:");
  while (ring_pop (&r, &x) == 0)
    printf (" %.0f", x);
  printf ("\n");

  /* A delay line: prime the ring with 3 zeros, then for each input push
     it and pop the oldest. What comes out is the input, 3 samples late. */
  for (int i = 0; i < 3; i++)
    ring_push (&r, 0.0f);
  printf ("in     :");
  for (int i = 1; i <= 10; i++)
    printf (" %2d", i);
  printf ("\ndelay 3:");
  for (int i = 1; i <= 10; i++)
    {
      ring_push (&r, (float)i);
      ring_pop (&r, &x);
      printf (" %2.0f", x);
    }
  printf ("\n");

  /* The counters only went up; the index went round. The 3 samples
     still waiting ARE the delay. */
  printf ("head %zu, tail %zu: %zu waiting, at slots %zu to %zu\n", r.head,
          r.tail, ring_count (&r), r.tail & (size_t)MASK,
          (r.head - (size_t)1) & (size_t)MASK);
  return 0;
}
