/* drift.c -- a phase kept three ways, for a minute.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       drift.c -o drift && ./drift
 *
 * Each phase adds its increment once per sample. Two wrap at the end
 * of each cycle, counting the wraps; one never wraps, and just grows.
 * The cycles counted over 60 seconds say what frequency each one really
 * played.
 */
#include <stdint.h>
#include <stdio.h>

static const double RATE    = 48000.0;
static const double FREQ    = 440.0;
static const long   SECONDS = 60;

int
main (void)
{
  long samples = SECONDS * (long)RATE;

  float fphase = 0.0f, finc = (float)(FREQ / RATE);
  long  fwraps = 0;
  for (long i = 0; i < samples; i++)
    {
      fphase += finc;
      if (fphase >= 1.0f) /* a cycle ends: wrap, and count it */
        {
          fphase -= 1.0f;
          fwraps++;
        }
    }

  /* The same float phase, never wrapped: it counts every cycle so far,
     and sin(2 pi phase) would still be the right wave, in exact math. */
  float grow = 0.0f;
  for (long i = 0; i < samples; i++)
    grow += finc;

  uint32_t uphase = 0, uinc = (uint32_t)(FREQ / RATE * 4294967296.0 + 0.5);
  long     uwraps = 0;
  for (long i = 0; i < samples; i++)
    {
      uint32_t before = uphase;
      uphase += uinc;
      if (uphase < before) /* it wrapped */
        uwraps++;
    }

  double secs = (double)SECONDS;
  printf ("asked for %.4f Hz, over %ld s:\n", FREQ, SECONDS);
  printf ("float, wrapped:       %.4f Hz\n",
          ((double)fwraps + (double)fphase) / secs);
  printf ("float, never wrapped: %.4f Hz\n", (double)grow / secs);
  printf ("uint32_t:             %.4f Hz\n",
          ((double)uwraps + (double)uphase / 4294967296.0) / secs);
  return 0;
}
