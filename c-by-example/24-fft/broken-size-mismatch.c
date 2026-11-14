/* broken-size-mismatch.c -- a buffer smaller than the transform.
 *
 * fft_forward trusts that x holds fft_size (p) samples: the plan, not
 * the buffer, says how many. Here the plan is for 1024 and the buffer
 * holds 1000 (a block size that "should be close enough"). The very
 * first step, the bit-reversed reordering, swaps samples anywhere up to
 * index 1023. This is that reordering, on its own, and the sanitizers
 * stop it at the first sample past the end.
 */
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>

static void
bit_reverse (double complex *x, size_t n)
{
  for (size_t i = 1, j = 0; i < n; i++)
    {
      size_t bit = n >> 1;
      for (; j & bit; bit >>= 1)
        j ^= bit;
      j ^= bit;
      if (i < j)
        {
          double complex t = x[i];
          x[i]             = x[j];
          x[j]             = t;
        }
    }
}

int
main (void)
{
  double complex *x = calloc (1000, sizeof *x); /* 1000 samples */
  if (x == NULL)
    return 1;
  bit_reverse (x, 1024); /* the plan's n */
  printf ("%f\n", creal (x[0]));
  free (x);
  return 0;
}
