/* fft.c -- an iterative radix-2 FFT. */
#include "fft.h"

#include <math.h>
#include <stdlib.h>

struct fft_plan
{
  size_t         n;
  double complex tw[]; /* n / 2 twiddle factors: e^(-2 pi i k / n) */
};

static const double PI = 3.141592653589793;

fft_plan *
fft_plan_create (size_t n)
{
  if (n < 2 || (n & (n - 1)) != 0) /* a power of two has one bit set */
    return NULL;
  fft_plan *p = malloc (sizeof *p + n / 2 * sizeof p->tw[0]);
  if (p == NULL)
    return NULL;
  p->n = n;
  /* Each from cexp, not by multiplying one phasor by itself, which
     drifts (lesson 23, exercise 3). */
  for (size_t k = 0; k < n / 2; k++)
    p->tw[k] = cexp (-2.0 * PI * I * (double)k / (double)n);
  return p;
}

void
fft_plan_destroy (fft_plan *p)
{
  free (p);
}

size_t
fft_size (const fft_plan *p)
{
  return p->n;
}

void
fft_forward (const fft_plan *p, double complex *x)
{
  size_t n = p->n;

  /* 1. Reorder: put x[i] at the index whose bits are i's bits reversed.
        After this, every pair, quad, ... the stages below combine sits
        side by side. */
  for (size_t i = 1, j = 0; i < n; i++)
    {
      size_t bit = n >> 1;
      for (; j & bit; bit >>= 1) /* add 1 to j, counting from the top */
        j ^= bit;
      j ^= bit;
      if (i < j)
        {
          double complex t = x[i];
          x[i]             = x[j];
          x[j]             = t;
        }
    }

  /* 2. Combine: transforms of length len/2 become transforms of length
        len, by BUTTERFLIES. Each takes one value a from the first half
        and one b from the second, turns b by a twiddle factor, and
        writes a + b and a - b. log2 n stages of n / 2 butterflies. */
  for (size_t len = 2; len <= n; len <<= 1)
    {
      size_t half = len / 2;
      size_t step = n / len; /* twiddle k for this length is tw[k*step] */
      for (size_t start = 0; start < n; start += len)
        for (size_t k = 0; k < half; k++)
          {
            double complex a    = x[start + k];
            double complex b    = x[start + k + half] * p->tw[k * step];
            x[start + k]        = a + b;
            x[start + k + half] = a - b;
          }
    }
}
