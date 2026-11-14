/* rotator.c -- an oscillator that only multiplies, and how it drifts.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       rotator.c -o rotator -lm && ./rotator
 *
 * Start a phasor at 1 and multiply it by w = e^(i 2 pi f / rate) every
 * sample: it turns at f, and its real part is a cosine. One complex
 * multiply per sample, no sin or cos at all. But every multiply rounds,
 * and nothing pulls the size back to exactly 1.
 */
#include <complex.h>
#include <math.h>
#include <stdio.h>

static const double PI = 3.141592653589793;

int
main (void)
{
  const long    steps = 10000000; /* about 3.5 minutes at 48 kHz */
  float complex w     = (float complex)cexp (I * 2.0 * PI * 440.0 / 48000.0);
  float complex plain = 1.0f, fixed = 1.0f;
  for (long n = 1; n <= steps; n++)
    {
      plain *= w;
      fixed *= w;
      if (n % 1000 == 0) /* every 1000 samples, pull the size back to 1 */
        {
          float m2 = crealf (fixed) * crealf (fixed)
                     + cimagf (fixed) * cimagf (fixed);
          fixed *= (3.0f - m2) / 2.0f; /* close to 1 / |fixed| near 1 */
        }
    }
  printf ("after %ld steps:\n", steps);
  printf ("left alone:   size off 1 by more than 0.001: %s\n",
          fabsf (cabsf (plain) - 1.0f) > 0.001f ? "yes" : "no");
  printf ("renormalised: size off 1 by less than 1e-5:  %s\n",
          fabsf (cabsf (fixed) - 1.0f) < 1e-5f ? "yes" : "no");
  return 0;
}
