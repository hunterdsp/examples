/* inplace.c -- the same filter, into a second buffer and in place.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       inplace.c sig.c -o inplace -lm && ./inplace
 *
 * sig.h promises that in and out may be the same buffer. A promise in a
 * header is worth a test: run two identical filters, one each way, and
 * compare every sample.
 */
#include "sig.h"

#include <stdio.h>

enum
{
  N = 256
};

int
main (void)
{
  sig_lowpass *a, *b;
  if (sig_lowpass_create (1000.0, 48000.0, &a) != SIG_OK)
    return 1;
  if (sig_lowpass_create (1000.0, 48000.0, &b) != SIG_OK)
    {
      sig_lowpass_destroy (a);
      return 1;
    }

  float in[N], out[N], buf[N];
  for (size_t i = 0; i < N; i++)
    in[i] = buf[i] = (i / (size_t)32) % (size_t)2 == 0 ? 1.0f : -1.0f;

  sig_lowpass_process (a, in, out, N);  /* separate buffers */
  sig_lowpass_process (b, buf, buf, N); /* in place */

  size_t differ = 0;
  for (size_t i = 0; i < N; i++)
    if (out[i] != buf[i])
      differ++;
  printf ("%d samples of a square wave: %zu differ\n", N, differ);

  sig_lowpass_destroy (a);
  sig_lowpass_destroy (b);
  return 0;
}
