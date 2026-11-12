/* broken-link.c -- declared, called, never defined.
 *
 * The declaration satisfies the COMPILER: it knows dsp_rms's shape, so
 * the call compiles. But no file in this build defines dsp_rms, so the
 * LINKER (lesson 00, stage 4) has a hole it cannot fill.
 */
#include <stddef.h>
#include <stdio.h>

float dsp_rms (const float *buf, size_t n);

int
main (void)
{
  float buf[2] = { 1, -1 };
  printf ("%f\n", dsp_rms (buf, 2));
  return 0;
}
