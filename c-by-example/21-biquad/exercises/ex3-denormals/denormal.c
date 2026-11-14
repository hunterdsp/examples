/* denormal.c -- a filter ringing down into the tiniest numbers.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. \
 *       denormal.c ../../biquad.c -o denormal -lm && ./denormal
 *   cc -std=c99 -Wall -Wextra -O2 -I ../.. \
 *       denormal.c ../../biquad.c -o denormal -lm && ./denormal time
 *
 * After an impulse, a biquad's state shrinks forever without reaching 0.
 * Below about 1.2e-38, a float can only be SUBNORMAL (or DENORMAL): a
 * special form for numbers too small for the usual one, which some
 * processors handle far more slowly. Silence after a sound can then cost
 * more than the sound did.
 */
#include "biquad.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N    = 48000, /* one second */
  RUNS = 5
};

static float in[N], out[N];

/* CPU seconds to filter one second of a constant, tiny input. */
static double
seconds (biquad_coeffs c, float level)
{
  for (size_t i = 0; i < N; i++)
    in[i] = level;
  double best = 1e30;
  for (int run = 0; run < RUNS; run++)
    {
      biquad_t f     = { .c = c };
      clock_t  start = clock ();
      for (int rep = 0; rep < 20; rep++)
        biquad_process (&f, in, out, N);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      best     = s < best ? s : best;
    }
  return best;
}

int
main (int argc, char **argv)
{
  biquad_coeffs c = biquad_lowpass (1000.0, 0.7071, 48000.0);

  /* An impulse, then silence: watch the state shrink. */
  biquad_t f = { .c = c };
  in[0]      = 1.0f;
  biquad_process (&f, in, out, N);
  in[0]        = 0.0f;
  size_t first = 0;
  for (size_t i = 0; i < N && first == 0; i++)
    if (fpclassify (out[i]) == FP_SUBNORMAL)
      first = i;
  printf ("output goes subnormal: %s, within 2000 samples: %s\n",
          first > 0 ? "yes" : "no", first > 0 && first < 2000 ? "yes" : "no");

  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      /* Two inputs far too quiet to hear: one still a normal float, one
         subnormal, which keeps the whole filter's state subnormal. */
      double normal    = seconds (c, 1e-30f);
      double subnormal = seconds (c, 1e-40f);
      double per       = 1e9 / (20.0 * (double)N);
      printf ("input at 1e-30 (normal):    %5.2f ns per sample\n",
              normal * per);
      printf ("input at 1e-40 (subnormal): %5.2f ns per sample\n",
              subnormal * per);
    }
  return 0;
}
