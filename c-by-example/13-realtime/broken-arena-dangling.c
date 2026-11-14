/* broken-arena-dangling.c -- a buffer used after its arena was freed.
 *
 * Every pointer arena_alloc hands out points into the arena's one block,
 * so arena_free ends ALL of them at once, however far away they were
 * kept. Here `out` outlives the arena: the write lands in freed memory,
 * and AddressSanitizer stops it.
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

int
main (void)
{
  arena_t arena;
  if (arena_init (&arena, 4096) != 0)
    return 1;
  float *out = arena_alloc (&arena, (size_t)256 * sizeof *out);
  if (out == NULL)
    {
      arena_free (&arena);
      return 1;
    }
  arena_free (&arena); /* teardown... */
  out[0] = 1.0f;       /* ...and one more write, too late */
  printf ("out[0] %.1f\n", out[0]);
  return 0;
}
