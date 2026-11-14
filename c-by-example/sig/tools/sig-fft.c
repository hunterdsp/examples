/* sig-fft -- list the frequencies in raw float samples from stdin.
 *
 *   sig-fft
 *
 * Reads every sample, takes the amplitude spectrum of the last 4096 (by
 * then any filter upstream has settled), and prints each frequency whose
 * amplitude is above 0.01, for samples at 48000 per second. The bins are
 * 48000 / 4096 = 11.71875 Hz apart.
 */
#include <sig/fft.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
  RATE = 48000,
  N    = 4096
};

int
main (void)
{
  /* Keep the last N samples: read in blocks of N into the second half
     of a 2N buffer, then slide the newest N down to the first half. */
  static float buf[2 * N];
  size_t       have = 0; /* samples in buf[0 .. N), at most N */
  size_t       n;
  while ((n = fread (buf + N, sizeof buf[0], N, stdin)) > 0)
    {
      size_t keep = have + n < N ? have : N - n; /* old ones to keep */
      memmove (buf, buf + have - keep, keep * sizeof buf[0]);
      memmove (buf + keep, buf + N, n * sizeof buf[0]);
      have = keep + n;
    }
  if (ferror (stdin))
    {
      perror ("sig-fft: read");
      return 1;
    }
  if (have < N)
    {
      fprintf (stderr, "sig-fft: need %d samples, got %zu\n", N, have);
      return 1;
    }

  sig_fft   *p;
  sig_status s = sig_fft_create (N, &p);
  if (s != SIG_OK)
    {
      fprintf (stderr, "sig-fft: %s\n", sig_strerror (s));
      return 1;
    }
  static float amp[N / 2 + 1];
  sig_fft_amplitude (p, buf, amp);
  for (size_t k = 0; k <= N / 2; k++)
    if (amp[k] > 0.01f)
      printf ("%8.2f Hz  %.3f\n", (double)k * (double)RATE / (double)N,
              (double)amp[k]);
  sig_fft_destroy (p);
  return 0;
}
