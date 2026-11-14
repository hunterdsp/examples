/* smooth.c -- two filters through an opaque type.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       smooth.c lowpass.c -o smooth && ./smooth
 *
 * main never sees inside a lowpass_t: it holds two pointers, and every
 * use goes through a function in lowpass.h. Each filter keeps its own
 * memory between calls, so a signal fed in blocks comes out the same as
 * one fed all at once.
 */
#include "lowpass.h"

#include <stdio.h>

enum
{
  N = 8
};

/* A step: silence, then a sudden 1.0, the hardest thing to smooth. */
static void
step (float *buf, size_t n)
{
  for (size_t i = 0; i < n; i++)
    buf[i] = 1.0f;
}

static void
show (const char *label, const float *buf, size_t n)
{
  printf ("%-22s", label);
  for (size_t i = 0; i < n; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
}

int
main (void)
{
  lowpass_t *fast = lowpass_create (0.5f);
  lowpass_t *slow = lowpass_create (0.1f);
  if (fast == NULL || slow == NULL)
    {
      lowpass_destroy (fast); /* both accept NULL: no `if` needed */
      lowpass_destroy (slow);
      return 1;
    }

  float buf[N];
  step (buf, N);
  lowpass_process (fast, buf, N);
  show ("fast (a = 0.5):", buf, N);

  step (buf, N);
  lowpass_process (slow, buf, N);
  show ("slow (a = 0.1):", buf, N);

  /* The same step again, as two blocks of 4, after a reset: */
  lowpass_reset (slow);
  step (buf, N);
  lowpass_process (slow, buf, N / 2);
  lowpass_process (slow, buf + N / 2, N / 2);
  show ("slow, in two blocks:", buf, N);

  lowpass_destroy (fast);
  lowpass_destroy (slow);
  return 0;
}
