/* broken-short-history.c -- a history one sample too short.
 *
 * An n-tap filter needs the last n inputs: the current one, and n - 1
 * before it. Counting only the n - 1 "past" samples is the classic
 * off-by-one (lesson 04). The buffer is one float short, so the first
 * time the ring reaches its last slot, reading or writing, it is past
 * the end, and AddressSanitizer stops the program there.
 */
#include <stdio.h>
#include <stdlib.h>

enum
{
  TAPS = 4
};

int
main (void)
{
  const float taps[TAPS] = { 0.25f, 0.25f, 0.25f, 0.25f };
  float      *hist       = calloc (TAPS - 1, sizeof *hist); /* one short */
  if (hist == NULL)
    return 1;

  size_t newest = 0;
  for (int i = 0; i < 8; i++)
    {
      newest       = newest + 1 < TAPS ? newest + 1 : 0;
      hist[newest] = 1.0f; /* slot TAPS - 1 doesn't exist */
      float  acc   = 0.0f;
      size_t j     = newest;
      for (size_t k = 0; k < TAPS; k++)
        {
          acc += taps[k] * hist[j];
          j = j > 0 ? j - 1 : TAPS - 1;
        }
      printf ("%.2f\n", (double)acc);
    }
  free (hist);
  return 0;
}
