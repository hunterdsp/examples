/* broken-swapped-buffers.c -- in and out, the wrong way round.
 *
 * sig_lowpass_process takes (f, in, out, n), and in is const. Swapping
 * the two would filter the output buffer's garbage into the input. The
 * const in the API turns that mistake into a compile error: passing a
 * const float * where a writable float * is wanted would let the
 * function write through it.
 */
#include "sig.h"

int
main (void)
{
  const float  in[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  float        out[4];
  sig_lowpass *f;
  if (sig_lowpass_create (1000.0, 48000.0, &f) != SIG_OK)
    return 1;
  sig_lowpass_process (f, out, in, 4); /* swapped */
  sig_lowpass_destroy (f);
  return 0;
}
