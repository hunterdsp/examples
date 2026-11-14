/* analyse.c -- a program built against an INSTALLED sig: the headers and
 * library under staging/, found with pkg-config, not ../sig.
 *
 * It does what `sig-tone 1 750 6000 | sig-filter lowpass 1500 | sig-fft`
 * does, in one program.
 */
#include <sig/sig.h>

#include <stdio.h>

enum
{
  RATE = 48000,
  N    = 4096
};

int
main (void)
{
  static float tone[RATE], x[RATE], amp[N / 2 + 1];
  sig_osc     *a, *b;
  sig_biquad  *lp;
  sig_fft     *fft;
  if (sig_osc_create (750.0, RATE, &a) != SIG_OK
      || sig_osc_create (6000.0, RATE, &b) != SIG_OK
      || sig_biquad_create (SIG_LOWPASS, 1500.0, 0.7071, RATE, &lp) != SIG_OK
      || sig_fft_create (N, &fft) != SIG_OK)
    return 1; /* a demo: the system frees what was made */

  sig_osc_process (a, x, RATE);
  sig_osc_process (b, tone, RATE);
  for (size_t i = 0; i < RATE; i++)
    x[i] = 0.5f * x[i] + 0.5f * tone[i];
  sig_biquad_process (lp, x, x, RATE);
  sig_fft_amplitude (fft, x + RATE - N, amp); /* the last N samples */

  printf ("sig %s, installed\n", sig_version ());
  for (size_t k = 0; k <= N / 2; k++)
    if (amp[k] > 0.01f)
      printf ("%8.2f Hz  %.3f\n", (double)k * (double)RATE / (double)N,
              (double)amp[k]);

  sig_fft_destroy (fft);
  sig_biquad_destroy (lp);
  sig_osc_destroy (b);
  sig_osc_destroy (a);
  return 0;
}
