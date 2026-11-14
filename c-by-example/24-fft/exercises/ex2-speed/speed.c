/* speed.c -- the DFT and the FFT, timed as n grows.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       speed.c ../../fft.c -o speed -lm && ./speed
 *   cc -std=c99 -Wall -Wextra -O2 -I ../.. \
 *       speed.c ../../fft.c -o speed -lm && ./speed time
 */
#include "fft.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const double PI = 3.141592653589793;

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

static double complex src[4096], x[4096], y[4096];

int
main (int argc, char **argv)
{
  int    timing  = argc > 1 && strcmp (argv[1], "time") == 0;
  size_t sizes[] = { 256, 1024, 4096 };
  for (size_t s = 0; s < (timing ? 3u : 1u); s++)
    {
      size_t    n = sizes[s];
      fft_plan *p = fft_plan_create (n);
      if (p == NULL)
        return 1;
      for (size_t i = 0; i < n; i++)
        src[i] = sin (0.01 * (double)(i * i));
      clock_t start = clock ();
      dft (src, y, n);
      double t_dft = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      start        = clock ();
      for (int rep = 0; rep < 1000; rep++) /* too quick to time once */
        {
          memcpy (x, src, n * sizeof x[0]); /* in place: start afresh */
          fft_forward (p, x);
        }
      double t_fft
          = (double)(clock () - start) / (double)CLOCKS_PER_SEC / 1000.0;
      double worst = 0.0;
      for (size_t k = 0; k < n; k++)
        worst = fmax (worst, cabs (x[k] - y[k]));
      printf ("n = %4zu: FFT matches DFT: %s", n, worst < 1e-8 ? "yes" : "no");
      if (timing)
        printf ("; DFT %8.3f ms, FFT %6.3f ms", t_dft * 1e3, t_fft * 1e3);
      printf ("\n");
      fft_plan_destroy (p);
    }
  return 0;
}
