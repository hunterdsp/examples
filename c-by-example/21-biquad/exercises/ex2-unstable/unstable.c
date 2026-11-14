/* unstable.c -- feedback that grows instead of dying away.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       unstable.c ../../biquad.c -o unstable -lm && ./unstable
 *
 * A biquad's feedback multiplies its past outputs by a1 and a2. If they
 * are too large, each echo of the impulse comes back bigger than the
 * last, and the output grows without end. The only change from a good
 * filter here is a2: 1.01 instead of 0.99.
 */
#include "biquad.h"

#include <math.h>
#include <stdio.h>

enum
{
  N     = 5000,
  BLOCK = 1000
};

static void
run (const char *label, float a2)
{
  static float x[N];
  x[0]       = 1.0f;
  biquad_t f = { .c = { .b0 = 1.0f, .a1 = -1.9f, .a2 = a2 } };
  biquad_process (&f, x, x, N);
  printf ("a2 = %.2f, %s; largest |y| per %d samples:", (double)a2, label,
          BLOCK);
  for (size_t b = 0; b < N; b += BLOCK)
    {
      float peak = 0.0f;
      for (size_t i = b; i < b + BLOCK; i++)
        peak = fabsf (x[i]) > peak ? fabsf (x[i]) : peak;
      printf (" %.0e", (double)peak);
    }
  printf ("\n");
}

int
main (void)
{
  run ("stable", 0.99f);
  run ("unstable", 1.01f);
  return 0;
}
