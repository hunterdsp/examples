/* signal.c -- a growing buffer of samples, and who owns its memory.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       signal.c -o signal && ./signal
 *
 * A signal_t owns two blocks of heap memory: the struct itself and the
 * samples it points at. One function makes them (signal_create), one
 * function gives them back (signal_destroy), and every signal_create is
 * matched by exactly one signal_destroy. That pairing is what OWNERSHIP
 * means in C: the language does not track it, so the code says it.
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

/* Hand the samples to the caller and destroy the rest. Ownership MOVES:
   afterwards the caller must free the returned block, and s is gone. */
static float *
signal_release (signal_t *s, size_t *len)
{
  float *data = s->data;
  *len        = s->len;
  free (s); /* the struct only: its samples now belong to the caller */
  return data;
}

int
main (void)
{
  signal_t *s = signal_create (4);
  if (s == NULL)
    return 1;

  printf ("capacity %zu", s->cap);
  for (int i = 0; i < 1000; i++)
    {
      size_t cap = s->cap;
      if (signal_push (s, (float)i) != 0)
        {
          signal_destroy (s); /* every exit path gives back what it owns */
          return 1;
        }
      if (s->cap != cap)
        printf (" -> %zu", s->cap);
    }
  printf ("\npushed %zu samples, capacity %zu\n", s->len, s->cap);

  size_t n;
  float *samples = signal_release (s, &n); /* s must not be used again */
  double sum     = 0.0;
  for (size_t i = 0; i < n; i++)
    sum += (double)samples[i];
  printf ("released %zu samples (sum %.0f); main owns them now\n", n, sum);
  free (samples);
  return 0;
}
