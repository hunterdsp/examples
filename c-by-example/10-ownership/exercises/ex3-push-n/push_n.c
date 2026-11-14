/* push_n.c (exercise 3) -- append a whole block, growing at most once.
 *
 * Pushing 1000 samples one at a time from a capacity of 4 grows the
 * buffer 8 times. Told the count up front, signal_push_n can work out the
 * capacity it needs, grow once, and copy the block in with one memcpy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memcpy */

typedef struct
{
  float *data; /* owned: freed by signal_destroy */
  size_t len;  /* samples in use */
  size_t cap;  /* samples allocated; len <= cap */
} signal_t;

/* Make an empty signal with room for cap samples (cap > 0). The caller
   owns the result and must pass it to signal_destroy. Returns NULL if
   there is not enough memory, having freed anything it did allocate. */
static signal_t *
signal_create (size_t cap)
{
  signal_t *s = malloc (sizeof *s);
  if (s == NULL)
    return NULL;
  s->data = malloc (cap * sizeof *s->data);
  if (s->data == NULL)
    {
      free (s); /* the second allocation failed: undo the first */
      return NULL;
    }
  s->len = 0;
  s->cap = cap;
  return s;
}

/* Free everything s owns, then s itself. Like free, it accepts NULL, so
   cleanup code never needs an `if`. */
static void
signal_destroy (signal_t *s)
{
  if (s == NULL)
    return;
  free (s->data);
  free (s);
}

/* Append x[0..n-1], growing (by doubling) at most once. Returns 0, or -1
   if out of memory, in which case s is unchanged. */
static int
signal_push_n (signal_t *s, const float *x, size_t n)
{
  size_t new_cap = s->cap;
  while (new_cap - s->len < n) /* room for n more? */
    new_cap = new_cap * (size_t)2;
  if (new_cap != s->cap)
    {
      float *grown = realloc (s->data, new_cap * sizeof *grown);
      if (grown == NULL)
        return -1;
      s->data = grown;
      s->cap  = new_cap;
    }
  memcpy (&s->data[s->len], x, n * sizeof *x);
  s->len = s->len + n;
  return 0;
}

int
main (void)
{
  static float block[1000]; /* static: 4000 bytes, kept off the stack */
  for (int i = 0; i < 1000; i++)
    block[i] = (float)i;

  signal_t *s = signal_create (4);
  if (s == NULL)
    return 1;
  size_t before = s->cap;
  if (signal_push_n (s, block, 1000) != 0)
    {
      signal_destroy (s);
      return 1;
    }
  printf ("pushed %zu samples in one call: capacity %zu -> %zu, "
          "one realloc\n",
          s->len, before, s->cap);
  printf ("last sample %.0f\n", s->data[s->len - 1]);
  signal_destroy (s);
  return 0;
}
