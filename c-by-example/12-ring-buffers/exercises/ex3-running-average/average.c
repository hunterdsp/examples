/* average.c (exercise 3) -- lesson 04's moving average, with a ring.
 *
 * The running sum (lesson 04, exercise 3) adds the newest sample and
 * subtracts the one leaving the window. The ring is what remembers which
 * sample that is: it holds exactly the last WINDOW inputs, and the one
 * popped is the one that has just left.
 */
#include <stdio.h>

enum
{
  WINDOW = 4, /* a power of two, for the mask */
  MASK   = WINDOW - 1
};

int
main (void)
{
  float in[] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  int   n    = (int)(sizeof in / sizeof in[0]);

  float    ring[WINDOW] = { 0 }; /* the window starts full of silence */
  unsigned head         = 0;
  float    sum          = 0.0f;

  printf ("out:");
  for (int i = 0; i < n; i++)
    {
      float oldest                = ring[head & (unsigned)MASK];
      ring[head & (unsigned)MASK] = in[i]; /* the newest takes its slot */
      head++;
      sum = sum + in[i] - oldest;
      printf (" %4.2f", sum / (float)WINDOW);
    }
  printf ("\n");
  return 0;
}
