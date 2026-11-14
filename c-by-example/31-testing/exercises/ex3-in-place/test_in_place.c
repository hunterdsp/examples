/* test_in_place.c -- sig_biquad_process promises that in and out may be
 * the same buffer. Filter the same noise both ways and compare.
 */
#include "check.h"

#include <sig/biquad.h>

#include <stdint.h>

enum
{
  N = 1000
};

int
main (int argc, char *argv[])
{
  (void)argc;
  /* Noise from a fixed linear congruential generator (the constants
     are Numerical Recipes'): the same numbers on every machine, which a
     test needs. Unsigned arithmetic wraps, so the overflow is defined. */
  static float in[N], out[N], buf[N];
  uint32_t     state = 1;
  for (size_t i = 0; i < N; i++)
    {
      state  = state * 1664525u + 1013904223u;
      in[i]  = (float)(state >> 8) / 16777216.0f - 0.5f;
      buf[i] = in[i];
    }

  sig_biquad *a, *b;
  CHECK (sig_biquad_create (SIG_HIGHPASS, 3000.0, 0.7071, 48000.0, &a)
         == SIG_OK);
  CHECK (sig_biquad_create (SIG_HIGHPASS, 3000.0, 0.7071, 48000.0, &b)
         == SIG_OK);
  if (a == NULL || b == NULL)
    return check_report (argv[0]);
  sig_biquad_process (a, in, out, N);  /* separate buffers */
  sig_biquad_process (b, buf, buf, N); /* in place */

  int same = 1;
  for (size_t i = 0; i < N; i++)
    same = same && out[i] == buf[i];
  CHECK (same);
  CHECK (in[0] != out[0]); /* and it did filter something */

  sig_biquad_destroy (a);
  sig_biquad_destroy (b);
  return check_report (argv[0]);
}
