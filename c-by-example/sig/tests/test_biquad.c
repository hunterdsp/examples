/* test_biquad.c -- the filters pass and stop what they should. */
#include "check.h"

#include <sig/biquad.h>
#include <sig/osc.h>

#include <math.h>

enum
{
  RATE = 48000,
  N    = 4800 /* a tenth of a second */
};

/* The amplitude of a sine at freq_hz after filter f: the largest output
   in the second half, once the filter has settled. */
static double
gain (sig_biquad *f, double freq_hz)
{
  static float buf[N];
  sig_osc     *o;
  if (sig_osc_create (freq_hz, RATE, &o) != SIG_OK)
    return -1.0;
  sig_osc_process (o, buf, N);
  sig_osc_destroy (o);
  sig_biquad_reset (f);
  sig_biquad_process (f, buf, buf, N); /* in place */
  double peak = 0.0;
  for (size_t i = N / 2; i < N; i++)
    peak = fabs ((double)buf[i]) > peak ? fabs ((double)buf[i]) : peak;
  return peak;
}

int
main (int argc, char *argv[])
{
  (void)argc;
  sig_biquad *f = (sig_biquad *)1;

  CHECK (sig_biquad_create (SIG_LOWPASS, 0.0, 0.7071, RATE, &f) == SIG_EINVAL
         && f == NULL);
  CHECK (sig_biquad_create (SIG_LOWPASS, 1000.0, 0.0, RATE, &f) == SIG_EINVAL
         && f == NULL);
  CHECK (sig_biquad_create ((sig_biquad_type)7, 1000.0, 0.7071, RATE, &f)
             == SIG_EINVAL
         && f == NULL);

  /* A Butterworth low-pass at 1000 Hz: flat well below the corner, 3 dB
     down (0.7071) at it, and falling 12 dB per octave above it. */
  CHECK (sig_biquad_create (SIG_LOWPASS, 1000.0, 0.7071, RATE, &f) == SIG_OK);
  if (f == NULL)
    return check_report (argv[0]);
  CHECK (fabs (gain (f, 100.0) - 1.0) < 0.01);
  CHECK (fabs (gain (f, 1000.0) - 0.7071) < 0.01);
  CHECK (gain (f, 8000.0) < 0.02);
  sig_biquad_destroy (f);

  /* The high-pass, the other way round. */
  CHECK (sig_biquad_create (SIG_HIGHPASS, 1000.0, 0.7071, RATE, &f) == SIG_OK);
  if (f == NULL)
    return check_report (argv[0]);
  CHECK (gain (f, 100.0) < 0.02);
  CHECK (fabs (gain (f, 1000.0) - 0.7071) < 0.01);
  CHECK (fabs (gain (f, 10000.0) - 1.0) < 0.02);

  /* reset forgets the past: an impulse gives the same response twice. */
  float a[8] = { 1.0f }, b[8] = { 1.0f };
  sig_biquad_reset (f);
  sig_biquad_process (f, a, a, 8);
  sig_biquad_reset (f);
  sig_biquad_process (f, b, b, 8);
  int same = 1;
  for (size_t i = 0; i < 8; i++)
    same = same && a[i] == b[i];
  CHECK (same);

  sig_biquad_destroy (f);
  return check_report (argv[0]);
}
