/* history.c (exercise 2) -- a 256-sample history that wraps by itself.
 *
 * A uint8_t holds 0..255, and unsigned arithmetic is modulo 2^8 when the
 * result is stored back (lesson 02). So with exactly 256 slots, a uint8_t
 * index needs no mask and no check: 255 + 1 stores as 0, and 0 - 1 as
 * 255. "The sample k steps ago" is just history[(uint8_t)(w - k)].
 */
#include <stdint.h>
#include <stdio.h>

int
main (void)
{
  static float history[256]; /* static: starts at zero */
  uint8_t      w = 0;        /* the next slot to write */

  for (int i = 0; i < 300; i++)
    {
      history[w] = (float)i;
      w          = (uint8_t)((unsigned)w + 1u); /* 255 + 1 stores as 0 */
    }
  printf ("wrote 300 samples into 256 slots; next write at slot %u\n",
          (unsigned)w);

  printf ("the last 4, newest first:");
  for (unsigned k = 1; k <= 4; k++)
    printf (" %.0f", history[(uint8_t)((unsigned)w - k)]);
  printf ("\n");

  uint8_t zero = 0;
  printf ("slot before 0: %u\n", (unsigned)(uint8_t)((unsigned)zero - 1u));
  return 0;
}
