/* rounding.c -- dropping bits by shifting, with and without rounding.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       rounding.c -o rounding -lm && ./rounding
 *
 * After a Q15 multiply, 15 bits must go. `p >> 15` simply drops them,
 * which rounds DOWN, every time. `(p + (1 << 14)) >> 15` adds half a
 * step first, which rounds to NEAREST. Measured over a whole sine wave,
 * in units of the last bit (1/32768):
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>

int
main (void)
{
  const int32_t g     = 9830; /* 0.3 in Q15 */
  double        trunc = 0.0, near = 0.0;
  const int     n = 48000;
  for (int i = 0; i < n; i++)
    {
      double  x = 0.9 * sin (6.283185307179586 * 440.0 * (double)i / 48000.0);
      int32_t q = (int32_t)lround (x * 32768.0);
      int32_t p = q * g;                   /* 30 bits after the point */
      double  exact = (double)p / 32768.0; /* in units of the last bit */
      trunc += (double)(p >> 15) - exact;  /* dropped */
      near += (double)((p + (1 << 14)) >> 15) - exact; /* rounded */
    }
  printf ("average error, truncated: %+.3f of the last bit\n",
          trunc / (double)n);
  near /= (double)n;
  if (fabs (near) < 0.0005) /* below the last digit printed: call it 0, */
    near = 0.0;             /* so no machine prints -0.000 */
  printf ("average error, rounded:   %+.3f of the last bit\n", near);
  return 0;
}
