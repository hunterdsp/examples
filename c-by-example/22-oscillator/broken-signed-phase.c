/* broken-signed-phase.c -- the phase accumulator in a SIGNED integer.
 *
 * osc.c's phase wraps at the end of each cycle because unsigned
 * arithmetic is defined to wrap. Signed arithmetic is not: overflowing
 * an int32_t is undefined behaviour (lesson 11). It may well wrap on
 * your machine, until an optimiser assumes it can't. UBSan stops the
 * program at the first overflow, about 55 samples in.
 */
#include <stdint.h>
#include <stdio.h>

int
main (void)
{
  int32_t phase = 0;
  int32_t inc   = 39370534; /* 440 Hz at 48 kHz, as in osc.c */
  for (int i = 0; i < 48000; i++)
    phase += inc; /* passes INT32_MAX within 55 samples */
  printf ("phase %d\n", (int)phase);
  return 0;
}
