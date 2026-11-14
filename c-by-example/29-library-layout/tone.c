/* tone.c -- a program that uses sig, the library, not a copy of its code.
 *
 * It includes the header from include/sig/ and links with libsig.a; no
 * line of the oscillator is compiled here.
 */
#include <sig/osc.h>

#include <stdio.h>

int
main (void)
{
  sig_osc   *osc;
  sig_status s = sig_osc_create (30000.0, 48000.0, &osc);
  printf ("30000 Hz at 48000 samples/s: %s\n", sig_strerror (s));

  s = sig_osc_create (750.0, 48000.0, &osc);
  if (s != SIG_OK)
    {
      fprintf (stderr, "tone: %s\n", sig_strerror (s));
      return 1;
    }
  float buf[8];
  sig_osc_process (osc, buf, 8);
  sig_osc_destroy (osc);

  printf ("750 Hz:");
  for (size_t i = 0; i < 8; i++)
    printf (" %.3f", (double)buf[i]);
  printf ("\n");
  return 0;
}
