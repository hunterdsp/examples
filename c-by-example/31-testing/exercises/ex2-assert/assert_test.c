/* assert_test.c -- a test written with assert(), and a wrong expectation:
 * an FFT made for 1024 samples is 1024 long, not 1000. It must fail.
 */
#include <sig/fft.h>

#include <assert.h>
#include <stdio.h>

int
main (void)
{
  sig_fft *p;
  if (sig_fft_create (1024, &p) != SIG_OK)
    return 1;
  assert (sig_fft_size (p) == 1000); /* wrong on purpose */
  sig_fft_destroy (p);
  printf ("all tests passed\n");
  return 0;
}
