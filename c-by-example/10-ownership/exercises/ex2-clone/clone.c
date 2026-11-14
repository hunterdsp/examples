/* clone.c (exercise 2) -- a deep copy: new memory, the same samples.
 *
 * `signal_t copy = *s` would copy the struct, and with it the ADDRESS in
 * data: two signals sharing one block, and two destroys freeing it twice
 * (broken-double-free.c). signal_clone allocates a block of its own and
 * copies the samples into it, so the clone owns its memory outright.
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

/* Append one sample, doubling the capacity when it is full. Returns 0, or
   -1 if there is no memory to grow, in which case s is unchanged. */
static int
signal_push (signal_t *s, float x)
{
  if (s->len == s->cap)
    {
      size_t new_cap = s->cap * (size_t)2;
      /* realloc into a TEMPORARY: on failure it returns NULL and leaves
         the old block alone, which s->data must still point at. */
      float *grown = realloc (s->data, new_cap * sizeof *grown);
      if (grown == NULL)
        return -1;
      s->data = grown; /* the old address may now be invalid */
      s->cap  = new_cap;
    }
  s->data[s->len] = x;
  s->len++;
  return 0;
}

/* A new signal with the same samples as s, owning its own memory. The
   caller must signal_destroy it. Returns NULL if out of memory. */
static signal_t *
signal_clone (const signal_t *s)
{
  signal_t *c = signal_create (s->cap);
  if (c == NULL)
    return NULL;
  memcpy (c->data, s->data, s->len * sizeof *s->data);
  c->len = s->len;
  return c;
}

static void
print (const char *label, const signal_t *s)
{
  printf ("%s:", label);
  for (size_t i = 0; i < s->len; i++)
    printf (" %.1f", s->data[i]);
  printf ("\n");
}

int
main (void)
{
  signal_t *a = signal_create (4);
  if (a == NULL)
    return 1;
  for (int i = 0; i < 4; i++)
    if (signal_push (a, (float)i) != 0)
      {
        signal_destroy (a);
        return 1;
      }

  signal_t *b = signal_clone (a);
  if (b == NULL)
    {
      signal_destroy (a);
      return 1;
    }
  b->data[0] = 9.0f;
  print ("original", a);
  print ("clone   ", b);
  printf ("separate blocks: %s\n", a->data != b->data ? "yes" : "no");
  signal_destroy (b); /* two creates (one inside clone), two destroys */
  signal_destroy (a);
  return 0;
}
