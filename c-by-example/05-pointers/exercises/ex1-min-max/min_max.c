/* min_max.c (exercise 1) -- two results, handed back through pointers.
 *
 * A function returns one value. To hand back two, the caller passes the
 * ADDRESSES of two variables, and the function writes into them: the same
 * trick as swap(), used for output.
 */
#include <stddef.h>
#include <stdio.h>

/* n must be at least 1: with no samples there is no minimum to report. */
static void
min_max (const float *buf, size_t n, float *lo, float *hi)
{
  *lo = buf[0];
  *hi = buf[0];
  for (size_t i = 1; i < n; i++)
    {
      if (buf[i] < *lo)
        *lo = buf[i];
      if (buf[i] > *hi)
        *hi = buf[i];
    }
}

int
main (void)
{
  float sig[] = { 0.5f, -1.0f, 2.0f, 0.3f };
  float lo, hi;
  min_max (sig, sizeof sig / sizeof sig[0], &lo, &hi);
  printf ("min %.2f, max %.2f\n", lo, hi);
  return 0;
}
