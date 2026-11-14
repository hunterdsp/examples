/* sig-filter -- filter raw float samples from stdin to stdout.
 *
 *   sig-filter lowpass|highpass FREQ_HZ
 *
 * A Butterworth biquad (q = 0.7071) with its corner at FREQ_HZ, for
 * samples at 48000 per second, read and written as raw native floats.
 */
#include <sig/biquad.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
  RATE  = 48000,
  BLOCK = 1024
};

int
main (int argc, char *argv[])
{
  if (argc != 3
      || (strcmp (argv[1], "lowpass") != 0
          && strcmp (argv[1], "highpass") != 0))
    {
      fprintf (stderr, "usage: sig-filter lowpass|highpass FREQ_HZ\n");
      return 2;
    }
  sig_biquad_type type
      = strcmp (argv[1], "lowpass") == 0 ? SIG_LOWPASS : SIG_HIGHPASS;
  char  *end;
  double f0 = strtod (argv[2], &end);
  if (end == argv[2] || *end != '\0')
    {
      fprintf (stderr, "sig-filter: %s: not a number\n", argv[2]);
      return 2;
    }

  sig_biquad *f;
  sig_status  s = sig_biquad_create (type, f0, 0.7071, RATE, &f);
  if (s != SIG_OK)
    {
      fprintf (stderr, "sig-filter: %s Hz: %s\n", argv[2], sig_strerror (s));
      return 1;
    }

  float  buf[BLOCK];
  size_t n;
  int    status = 0;
  while ((n = fread (buf, sizeof buf[0], BLOCK, stdin)) > 0)
    {
      sig_biquad_process (f, buf, buf, n); /* in place */
      if (fwrite (buf, sizeof buf[0], n, stdout) != n)
        {
          perror ("sig-filter: write");
          status = 1;
          break;
        }
    }
  if (ferror (stdin))
    {
      perror ("sig-filter: read");
      status = 1;
    }
  sig_biquad_destroy (f);
  return status;
}
