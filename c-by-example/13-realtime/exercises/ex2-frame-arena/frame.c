/* frame.c (exercise 2) -- scratch memory that lives for one block.
 *
 * Some scratch is only needed while one block is processed. A second,
 * SCRATCH arena serves it: allocate from it freely during the block, and
 * reset it (used = 0) at the end, which frees everything at once by
 * forgetting it. Still one malloc, at setup; every block starts with the
 * whole scratch arena free again.
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

/* Forgets every piece: the next arena_alloc starts from the front. Every
   pointer it handed out must be finished with. Costs one store. */
static void
arena_reset (arena_t *a)
{
  a->used = 0;
}

/* Frees every piece at once. Each pointer arena_alloc gave is now dead. */
static void
arena_free (arena_t *a)
{
  free (a->base);
  a->base = NULL;
}

typedef struct
{
  float gain;
} gain_t;

/* The ramp comes from the scratch arena, which the caller resets after
   the block. No malloc: arena_alloc is an addition and a comparison. */
static int
gain_process (gain_t *g, arena_t *scratch, const float *in, float *out,
              size_t n, float target)
{
  float *ramp = arena_alloc (scratch, n * sizeof *ramp);
  if (ramp == NULL)
    return -1;
  float step = (target - g->gain) / (float)n;
  for (size_t i = 0; i < n; i++)
    ramp[i] = g->gain + step * ((float)i + 1.0f);
  for (size_t i = 0; i < n; i++)
    out[i] = in[i] * ramp[i];
  g->gain = target;
  return 0;
}

int
main (void)
{
  static float in[256], out[256];
  for (size_t i = 0; i < (size_t)256; i++)
    in[i] = 1.0f;
  gain_t g = { 1.0f };

  arena_t scratch; /* room for the largest block's scratch */
  if (arena_init (&scratch, (size_t)256 * sizeof (float)) != 0)
    return 1;
  size_t before = n_mallocs;

  size_t sizes[3] = { 64, 128, 256 };
  size_t n = 0, peak = 0;
  for (int b = 0; b < 1000; b++)
    {
      n = sizes[b % 3];
      if (gain_process (&g, &scratch, in, out, n, b % 2 == 0 ? 0.5f : 1.0f)
          != 0)
        {
          arena_free (&scratch);
          return 1;
        }
      if (scratch.used > peak)
        peak = scratch.used;
      arena_reset (&scratch); /* the block is done: so is its scratch */
    }
  printf ("1000 blocks processed: %zu mallocs\n", n_mallocs - before);
  printf ("scratch: peak %zu of %zu bytes, %zu after each reset\n", peak,
          scratch.size, scratch.used);
  printf ("last block: out[0] %.4f, out[%zu] %.4f\n", out[0], n - (size_t)1,
          out[n - (size_t)1]);
  arena_free (&scratch);
  return 0;
}
