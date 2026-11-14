/* api.c -- a program written against sig.h, and nothing else.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       api.c sig.c -o api -lm && ./api
 */
#include "sig.h"

#include <stdio.h>

enum
{
  N = 8
};

int
main (void)
{
  printf ("built against sig %d.%d.%d, running sig %s\n", SIG_VERSION_MAJOR,
          SIG_VERSION_MINOR, SIG_VERSION_PATCH, sig_version ());

  /* A mistake: 30 kHz is above half of 48 kHz. The status says so. */
  sig_lowpass *f;
  sig_status   s = sig_lowpass_create (30000.0, 48000.0, &f);
  printf ("30 kHz at 48 kHz: %s\n", sig_strerror (s));

  s = sig_lowpass_create (1000.0, 48000.0, &f);
  if (s != SIG_OK)
    {
      fprintf (stderr, "sig_lowpass_create: %s\n", sig_strerror (s));
      return 1;
    }

  const float in[N] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  float       out[N];
  sig_lowpass_process (f, in, out, N); /* in is const: left as it was */
  printf (" 1 kHz at 48 kHz:");
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)out[i]);
  printf ("\n");

  sig_lowpass_destroy (f);
  return 0;
}
