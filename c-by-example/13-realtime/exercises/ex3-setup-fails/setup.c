/* setup.c (exercise 3) -- an arena too small, found at setup.
 *
 * realtime.c's setup with a choice of largest block. It needs three
 * buffers of that many floats: for 1024 samples, 12288 bytes, three
 * times the arena's 4096. Then arena_alloc returns NULL, and setup says
 * so and stops: before any audio runs, which is when a failure costs
 * nothing.
 */
#include <stdio.h>
#include <stdlib.h>

static size_t n_mallocs; /* every malloc this program makes */

static void *
counted_malloc (size_t bytes)
{
  n_mallocs++;
  return malloc (bytes);
}

/* An arena: one block, handed out in pieces from the front, and freed
   all at once. Allocating is an addition and a comparison: always the
   same, short time. */
typedef struct
{
  unsigned char *base; /* the block: bytes, so offsets count bytes */
  size_t         size; /* its size */
  size_t         used; /* bytes handed out so far */
} arena_t;

/* Every piece starts at a multiple of 16 bytes, enough for any type in
   this course (malloc's own block is aligned for any type). */
enum
{
  ALIGN = 16
};

static int
arena_init (arena_t *a, size_t size)
{
  a->base = counted_malloc (size);
  a->size = size;
  a->used = 0;
  return a->base != NULL ? 0 : -1;
}

/* Returns `bytes` from the arena, or NULL if it has too few left. */
static void *
arena_alloc (arena_t *a, size_t bytes)
{
  /* Round the start up to the next multiple of ALIGN: add ALIGN - 1,
     then clear the low bits (ALIGN is a power of two, lesson 12). */
  size_t start
      = (a->used + ((size_t)ALIGN - (size_t)1)) & ~((size_t)ALIGN - (size_t)1);
  if (start > a->size || bytes > a->size - start)
    return NULL;
  a->used = start + bytes;
  return a->base + start;
}

/* Frees every piece at once. Each pointer arena_alloc gave is now dead. */
static void
arena_free (arena_t *a)
{
  free (a->base);
  a->base = NULL;
}

/* A gain stage that changes gain without a click: over one block it
   ramps from the old gain to the new. The ramp lives in a scratch buffer
   sized, at setup, for the largest block it will ever be given. */
typedef struct
{
  float  gain;
  size_t max_block;
  float *ramp; /* max_block floats, from the arena */
} gain_t;

static int
gain_init (gain_t *g, arena_t *a, size_t max_block)
{
  g->gain      = 1.0f;
  g->max_block = max_block;
  g->ramp      = arena_alloc (a, max_block * sizeof *g->ramp);
  return g->ramp != NULL ? 0 : -1;
}

/* Everything realtime.c allocates, for blocks of up to max_block.
   Returns 0, or -1 having freed what it got, if the arena is too small. */
static int
setup (arena_t *arena, gain_t *g, float **in, float **out, size_t max_block)
{
  if (arena_init (arena, 4096) != 0)
    return -1;
  *in  = arena_alloc (arena, max_block * sizeof **in);
  *out = arena_alloc (arena, max_block * sizeof **out);
  if (*in == NULL || *out == NULL || gain_init (g, arena, max_block) != 0)
    {
      arena_free (arena);
      return -1;
    }
  return 0;
}

int
main (void)
{
  size_t tries[2] = { 256, 1024 };
  for (int t = 0; t < 2; t++)
    {
      arena_t arena;
      gain_t  g;
      float  *in, *out;
      size_t  need = (size_t)3 * tries[t] * sizeof (float);
      if (setup (&arena, &g, &in, &out, tries[t]) == 0)
        {
          printf ("max block %4zu: needs %5zu bytes, setup ok (%zu used)\n",
                  tries[t], need, arena.used);
          arena_free (&arena);
        }
      else
        printf ("max block %4zu: needs %5zu bytes, setup refused\n", tries[t],
                need);
    }
  return 0;
}
