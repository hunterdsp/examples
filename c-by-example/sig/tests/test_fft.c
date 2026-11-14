/* test_fft.c -- each tone shows up in its own bin, at its amplitude. */
#include "check.h"

#include <sig/fft.h>
#include <sig/osc.h>

#include <math.h>

enum
{
  N = 1024
};

int
main (int argc, char *argv[])
{
  (void)argc;
  sig_fft *p = (sig_fft *)1;

  CHECK (sig_fft_create (0, &p) == SIG_EINVAL && p == NULL);
  CHECK (sig_fft_create (1, &p) == SIG_EINVAL && p == NULL);
  CHECK (sig_fft_create (1000, &p) == SIG_EINVAL && p == NULL);

  CHECK (sig_fft_create (N, &p) == SIG_OK && p != NULL);
  if (p == NULL)
    return check_report (argv[0]);
  CHECK (sig_fft_size (p) == N);

  /* 0.25 of DC, plus a sine of amplitude 0.5 exactly on bin 64. With
     the rate equal to N, bin k is k Hz. */
  static float in[N], amp[N / 2 + 1];
  sig_osc     *o;
  CHECK (sig_osc_create (64.0, N, &o) == SIG_OK);
  if (o == NULL)
    return check_report (argv[0]);
  sig_osc_process (o, in, N);
  sig_osc_destroy (o);
  for (size_t i = 0; i < N; i++)
    in[i] = 0.25f + 0.5f * in[i];

  sig_fft_amplitude (p, in, amp);
  CHECK (fabs ((double)amp[0] - 0.25) < 1e-4);
  CHECK (fabs ((double)amp[64] - 0.5) < 1e-4);
  double rest = 0.0; /* the largest amplitude anywhere else */
  for (size_t k = 1; k <= N / 2; k++)
    if (k != 64 && (double)amp[k] > rest)
      rest = (double)amp[k];
  CHECK (rest < 1e-4);

  sig_fft_destroy (p);
  return check_report (argv[0]);
}
