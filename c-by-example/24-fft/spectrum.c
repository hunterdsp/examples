/* spectrum.c -- the spectrum of a signal, by FFT, checked by DFT.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       spectrum.c fft.c -o spectrum -lm && ./spectrum
 *
 * Two tones, transformed. The FFT's answer is compared with the DFT's,
 * computed straight from its definition, the slow way.
 */
#include "fft.h"

#include <math.h>
#include <stdio.h>

enum
{
  N = 1024
};

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

/* The DFT by its definition: bin k is the sum of x[i] e^(-2 pi i k i/n). */
static void
dft (const double complex *x, double complex *out, size_t n)
{
  for (size_t k = 0; k < n; k++)
    {
      double complex sum = 0.0;
      for (size_t i = 0; i < n; i++)
        sum += x[i] * cexp (-2.0 * PI * I * (double)((k * i) % n) / (double)n);
      out[k] = sum;
    }
}

int
main (void)
{
  fft_plan *p = fft_plan_create (N);
  if (p == NULL)
    return 1;

  /* Bin k is k * RATE / N Hz: 46.875 Hz apart. Two tones that land
     exactly on bins 20 and 64: 937.5 Hz, size 0.5, and 3000 Hz, 0.2. */
  static double complex x[N], slow[N];
  for (size_t i = 0; i < N; i++)
    x[i] = 0.5 * cos (2.0 * PI * 20.0 * (double)i / (double)N)
           + 0.2 * cos (2.0 * PI * 64.0 * (double)i / (double)N);
  dft (x, slow, N);
  fft_forward (p, x);

  double worst = 0.0;
  for (size_t k = 0; k < N; k++)
    worst = fmax (worst, cabs (x[k] - slow[k]));
  printf ("FFT and DFT agree within 1e-9: %s\n", worst < 1e-9 ? "yes" : "no");

  /* Bins up to N/2 (half the sample rate) are the frequencies; the rest
     mirror them, for a real signal. A cosine of size A shows as N*A/2. */
  printf ("bins with anything in them, up to %.0f Hz:\n", RATE / 2.0);
  for (size_t k = 0; k <= N / 2; k++)
    {
      double size = cabs (x[k]) * 2.0 / (double)N;
      if (size > 1e-6)
        printf ("  bin %3zu = %6.1f Hz: size %.4f\n", k,
                (double)k * RATE / (double)N, size);
    }

  printf ("complex multiplies: DFT %zu, FFT %zu\n", (size_t)N * N,
          (size_t)N / 2 * 10); /* log2 1024 = 10 stages */
  fft_plan_destroy (p);
  return 0;
}
