/* fft.c -- an iterative radix-2 FFT (lesson 24), real in, amplitude out.
 *
 * <complex.h> is used here, inside the library, and nowhere in its
 * headers: a caller needs no complex numbers to use it.
 */
#include <sig/fft.h>

#include "internal.h"

#include <complex.h>
#include <math.h>
#include <stdlib.h>

struct sig_fft
{
  size_t          n;
  double complex *tw;    /* n / 2 twiddle factors: e^(-2 pi i k / n) */
  double complex *work;  /* n values: the transform happens here */
  double complex  buf[]; /* tw and work, in one allocation */
};

sig_status
sig_fft_create (size_t n, sig_fft **out)
{
  *out = NULL;
  if (n < 2 || (n & (n - 1)) != 0) /* a power of two has one bit set */
    return SIG_EINVAL;
  sig_fft *p = malloc (sizeof *p + (n / 2 + n) * sizeof p->buf[0]);
  if (p == NULL)
    return SIG_ENOMEM;
  p->n    = n;
  p->tw   = p->buf;
  p->work = p->buf + n / 2;
  for (size_t k = 0; k < n / 2; k++)
    p->tw[k] = cexp (-2.0 * SIG_PI * I * (double)k / (double)n);
  *out = p;
  return SIG_OK;
}

void
sig_fft_destroy (sig_fft *p)
{
  free (p);
}

size_t
sig_fft_size (const sig_fft *p)
{
  return p->n;
}

/* Transform x in place: afterwards x[k] is bin k. */
static void
transform (const sig_fft *p, double complex *x)
{
  size_t n = p->n;

  /* 1. Reorder: x[i] goes to the index whose bits are i's reversed. */
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

  /* 2. Combine, by butterflies: log2 n stages of n / 2 each. */
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

void
sig_fft_amplitude (sig_fft *p, const float *in, float *amp)
{
  size_t n = p->n;
  for (size_t i = 0; i < n; i++)
    p->work[i] = (double)in[i];
  transform (p, p->work);
  /* A sine of amplitude A on bin k puts n A / 2 in bin k and the same in
     its mirror image, bin n - k (lesson 24). Bins 0 and n / 2 have no
     mirror, so they hold n A. */
  for (size_t k = 0; k <= n / 2; k++)
    {
      double scale
          = (k == 0 || k == n / 2) ? 1.0 / (double)n : 2.0 / (double)n;
      amp[k] = (float)(cabs (p->work[k]) * scale);
    }
}
