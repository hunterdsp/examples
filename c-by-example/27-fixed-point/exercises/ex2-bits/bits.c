/* bits.c -- how much noise rounding to b bits adds.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       bits.c -o bits -lm && ./bits
 *
 * A gain of 0.3 on a sine, with the signal, the gain and the result
 * each rounded to b bits after the binary point, as a Qb format does.
 */
#include <math.h>
#include <stdio.h>

/* x rounded to the nearest multiple of 2^-b. */
static double
quantise (double x, int b)
{
  double scale = ldexp (1.0, b); /* 2^b */
  return round (x * scale) / scale;
}

int
main (void)
{
  const int bits[] = { 7, 11, 15, 23 };
  printf ("bits  format   error below signal\n");
  for (size_t k = 0; k < sizeof bits / sizeof bits[0]; k++)
    {
      int    b   = bits[k];
      double sig = 0.0, err = 0.0;
      for (int i = 0; i < 48000; i++)
        {
          double x
              = 0.9 * sin (6.283185307179586 * 440.0 * (double)i / 48000.0);
          double exact = 0.3 * x;
          double fixed = quantise (quantise (x, b) * quantise (0.3, b), b);
          sig += exact * exact;
          err += (fixed - exact) * (fixed - exact);
        }
      printf ("%4d  Q%-2d %13.0f dB\n", b, b, -10.0 * log10 (err / sig));
    }
  return 0;
}
