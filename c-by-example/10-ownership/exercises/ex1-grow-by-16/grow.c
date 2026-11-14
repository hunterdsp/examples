/* grow.c (exercise 1) -- doubling versus growing by a fixed step.
 *
 * Every time the buffer is full, realloc may have to move it, copying
 * every sample so far. Growing by 16 at a time, 1000 samples need 63
 * moves; doubling needs 8. Doubling copies each sample about once on
 * average, however long the signal: that is why it is the usual choice.
 */
#include <stdio.h>
#include <stdlib.h>

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

/* Append one sample, growing by doubling (double_it != 0) or by 16
   samples when full. Counts each growth, and the samples a move would
   copy, in *grows and *copied. Returns 0, or -1 if out of memory. */
static int
signal_push (signal_t *s, float x, int double_it, size_t *grows,
             size_t *copied)
{
  if (s->len == s->cap)
    {
      size_t new_cap = double_it ? s->cap * (size_t)2 : s->cap + (size_t)16;
      float *grown   = realloc (s->data, new_cap * sizeof *grown);
      if (grown == NULL)
        return -1;
      s->data = grown;
      s->cap  = new_cap;
      *grows  = *grows + (size_t)1;
      *copied = *copied + s->len; /* at worst, realloc copies them all */
    }
  s->data[s->len] = x;
  s->len++;
  return 0;
}

static int
fill (const char *label, int double_it)
{
  signal_t *s = signal_create (4);
  if (s == NULL)
    return -1;
  size_t grows = 0, copied = 0;
  for (int i = 0; i < 1000; i++)
    if (signal_push (s, (float)i, double_it, &grows, &copied) != 0)
      {
        signal_destroy (s);
        return -1;
      }
  printf ("%s: %zu samples, %2zu growths, capacity %4zu, up to %5zu "
          "samples copied\n",
          label, s->len, grows, s->cap, copied);
  signal_destroy (s);
  return 0;
}

int
main (void)
{
  if (fill ("doubling", 1) != 0 || fill ("+16     ", 0) != 0)
    return 1;
  return 0;
}
