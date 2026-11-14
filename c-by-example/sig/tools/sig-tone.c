/* sig-tone -- write sine waves to stdout, as raw float samples.
 *
 *   sig-tone SECONDS FREQ_HZ...
 *
 * Writes SECONDS of the sum of one sine per FREQ_HZ, each of amplitude
 * 0.5, at 48000 samples per second, as raw native floats: the format
 * sig-filter reads and writes, and sig-fft reads.
 */
#include <sig/osc.h>

#include <stdio.h>
#include <stdlib.h>

enum
{
  RATE  = 48000,
  BLOCK = 1024,
  MAX   = 8 /* tones */
};

/* Parse s as a number, or say why not and exit. */
static double
number (const char *s)
{
  char  *end;
  double x = strtod (s, &end);
  if (end == s || *end != '\0')
    {
      fprintf (stderr, "sig-tone: %s: not a number\n", s);
      exit (2);
    }
  return x;
}

int
main (int argc, char *argv[])
{
  if (argc < 3 || argc - 2 > MAX)
    {
      fprintf (stderr, "usage: sig-tone SECONDS FREQ_HZ... (up to %d)\n", MAX);
      return 2;
    }
  double seconds = number (argv[1]);
  int    tones   = argc - 2;

  sig_osc *osc[MAX] = { NULL };
  int      status   = 0;
  for (int t = 0; t < tones && status == 0; t++)
    {
      sig_status s = sig_osc_create (number (argv[2 + t]), RATE, &osc[t]);
      if (s != SIG_OK)
        {
          fprintf (stderr, "sig-tone: %s Hz: %s\n", argv[2 + t],
                   sig_strerror (s));
          status = 1;
        }
    }

  long  left = seconds > 0.0 ? (long)(seconds * (double)RATE + 0.5) : 0;
  float sum[BLOCK], one[BLOCK];
  while (status == 0 && left > 0)
    {
      size_t n = left < BLOCK ? (size_t)left : BLOCK;
      for (size_t i = 0; i < n; i++)
        sum[i] = 0.0f;
      for (int t = 0; t < tones; t++)
        {
          sig_osc_process (osc[t], one, n);
          for (size_t i = 0; i < n; i++)
            sum[i] += 0.5f * one[i];
        }
      if (fwrite (sum, sizeof sum[0], n, stdout) != n)
        {
          perror ("sig-tone: write");
          status = 1;
        }
      left -= (long)n;
    }

  for (int t = 0; t < tones; t++)
    sig_osc_destroy (osc[t]);
  return status;
}
