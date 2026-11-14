/* copy.c (exercise 2) -- a copy that outlives the function that made it.
 *
 * signal_copy allocates on the heap, so the copy is still there after it
 * returns, and belongs to the caller: whoever calls signal_copy must
 * free what it returns. Changing the copy leaves the original alone.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memcpy */

/* Returns a heap copy of in[0..n-1] that the caller must free, or NULL
   if there is not enough memory. */
static float *
signal_copy (const float *in, size_t n)
{
  float *out = malloc (n * sizeof *out);
  if (out == NULL)
    return NULL;
  memcpy (out, in, n * sizeof *out); /* bytes, not elements */
  return out;
}

int
main (void)
{
  float  sig[] = { 0.0f, 0.5f, 1.0f, 0.5f };
  size_t n     = sizeof sig / sizeof sig[0];

  float *copy = signal_copy (sig, n);
  if (copy == NULL)
    return 1;
  for (size_t i = 0; i < n; i++)
    copy[i] = copy[i] * 2.0f;

  printf ("original:");
  for (size_t i = 0; i < n; i++)
    printf (" %.1f", sig[i]);
  printf ("\ncopy x2: ");
  for (size_t i = 0; i < n; i++)
    printf (" %.1f", copy[i]);
  printf ("\n");
  free (copy);
  return 0;
}
