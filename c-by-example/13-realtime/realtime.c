/* realtime.c -- allocate everything up front, then nothing per block.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       realtime.c -o realtime && ./realtime
 *
 * An audio callback has a deadline: 256 samples at 48 kHz must be ready
 * in 5.3 ms, every time, or the listener hears a click. malloc gives no
 * such promise: it may search, take a lock another thread holds, or ask
 * the operating system for memory. So a real-time program allocates at
 * SETUP, and its processing code only uses what it was given.
 *
 * Here setup is one malloc, carved into buffers by an ARENA, and the
 * program counts every malloc it makes to prove processing makes none.
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

/* The real-time part: no malloc, no free, no printf, no file, no lock.
   A block bigger than setup allowed is refused, not allocated for. */
static int
gain_process (gain_t *g, const float *in, float *out, size_t n, float target)
{
  if (n == 0 || n > g->max_block)
    return -1;
  float step = (target - g->gain) / (float)n;
  for (size_t i = 0; i < n; i++)
    g->ramp[i] = g->gain + step * ((float)i + 1.0f);
  for (size_t i = 0; i < n; i++)
    out[i] = in[i] * g->ramp[i];
  g->gain = target;
  return 0;
}

int
main (void)
{
  enum
  {
    MAX_BLOCK = 256
  };

  /* ---- setup: every allocation happens here ---- */
  arena_t arena;
  if (arena_init (&arena, 4096) != 0)
    return 1;
  gain_t g;
  float *in  = arena_alloc (&arena, (size_t)MAX_BLOCK * sizeof *in);
  float *out = arena_alloc (&arena, (size_t)MAX_BLOCK * sizeof *out);
  if (in == NULL || out == NULL
      || gain_init (&g, &arena, (size_t)MAX_BLOCK) != 0)
    {
      arena_free (&arena);
      return 1;
    }
  for (size_t i = 0; i < (size_t)MAX_BLOCK; i++)
    in[i] = 1.0f; /* a constant input shows the gain itself */
  printf ("setup: %zu malloc, 3 buffers carved from it (%zu of %zu bytes)\n",
          n_mallocs, arena.used, arena.size);

  /* ---- processing: blocks of 64, 128 and 256, gain 0.5 and 1 ---- */
  size_t before   = n_mallocs;
  size_t sizes[3] = { 64, 128, 256 };
  size_t n        = 0;
  float  from = 0.0f, to = 0.0f;
  for (int b = 0; b < 1000; b++)
    {
      n    = sizes[b % 3];
      from = g.gain;
      to   = b % 2 == 0 ? 0.5f : 1.0f;
      if (gain_process (&g, in, out, n, to) != 0)
        {
          arena_free (&arena);
          return 1;
        }
    }
  printf ("1000 blocks processed: %zu mallocs\n", n_mallocs - before);
  printf ("last block, %zu samples, ramps %.1f -> %.1f: out[0] %.4f, "
          "out[%zu] %.4f\n",
          n, from, to, out[0], n - (size_t)1, out[n - (size_t)1]);

  printf ("a block of 512: %s\n",
          gain_process (&g, in, out, (size_t)512, 1.0f) != 0
              ? "refused (max 256)"
              : "processed");
  arena_free (&arena); /* ---- teardown: one free for everything ---- */
  return 0;
}
