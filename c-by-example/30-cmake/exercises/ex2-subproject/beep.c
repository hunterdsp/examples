/* beep.c -- one program, built together with the library it uses. */
#include <sig/sig.h>

#include <stdio.h>

enum
{
  N = 4096
};

int
main (void)
{
  sig_osc *osc;
  sig_fft *fft;
  if (sig_osc_create (1500.0, 48000.0, &osc) != SIG_OK)
    return 1;
  if (sig_fft_create (N, &fft) != SIG_OK)
    {
      sig_osc_destroy (osc);
      return 1;
    }
  static float x[N], amp[N / 2 + 1];
  sig_osc_process (osc, x, N);
  sig_fft_amplitude (fft, x, amp);
  /* 1500 Hz is bin 1500 * 4096 / 48000 = 128. */
  printf ("sig %s: bin 128 has amplitude %.3f\n", sig_version (),
          (double)amp[128]);
  sig_fft_destroy (fft);
  sig_osc_destroy (osc);
  return 0;
}
