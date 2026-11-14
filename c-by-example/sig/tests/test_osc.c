/* test_osc.c -- the oscillator plays the frequency asked for, cleanly. */
#include "check.h"

#include <sig/osc.h>

#include <math.h>

enum
{
  RATE = 48000
};

int
main (int argc, char *argv[])
{
  (void)argc;
  sig_osc *o = (sig_osc *)1; /* not NULL, so the test sees it cleared */

  /* Arguments out of range are refused, and *out is NULL. */
  CHECK (sig_osc_create (0.0, RATE, &o) == SIG_EINVAL && o == NULL);
  CHECK (sig_osc_create (24000.0, RATE, &o) == SIG_EINVAL && o == NULL);
  CHECK (sig_osc_create (440.0, 0.0, &o) == SIG_EINVAL && o == NULL);
  CHECK (sig_osc_create (NAN, RATE, &o) == SIG_EINVAL && o == NULL);

  /* One second of 750 Hz, in uneven blocks: within 1e-5 of sin(), so
     each block carries on exactly where the last one stopped. 750 Hz is
     1/64 of the rate, exactly 2^26 steps of phase per sample; a
     frequency that isn't a whole number of steps plays slightly off,
     and drifts away from sin() (lesson 22). */
  CHECK (sig_osc_create (750.0, RATE, &o) == SIG_OK && o != NULL);
  if (o == NULL)
    return check_report (argv[0]);
  static float out[RATE];
  for (size_t done = 0, n = 1; done < RATE; done += n, n = n * 2 + 1)
    sig_osc_process (o, out + done, n < RATE - done ? n : RATE - done);
  double worst = 0.0;
  for (size_t i = 0; i < RATE; i++)
    {
      double want = sin (6.283185307179586 * 750.0 * (double)i / (double)RATE);
      double d    = fabs ((double)out[i] - want);
      worst       = d > worst ? d : worst;
    }
  CHECK (worst < 1e-5);
  CHECK (out[0] == 0.0f); /* phase 0 starts at 0 */

  sig_osc_destroy (o);
  sig_osc_destroy (NULL); /* allowed, and does nothing */
  return check_report (argv[0]);
}
