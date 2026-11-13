/* average.c (exercise 1) -- the same moving average, over 8 samples.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       average.c -o average && ./average
 *
 * Each output is the mean of the last WINDOW inputs, so a sudden jump in
 * the input becomes a ramp in the output: the filter smooths.
 */
#include <stdio.h>

/* A count known at compile time. An enum constant, unlike a const int,
   can size an array in C99. */
enum { WINDOW = 8 }; /* exercise 1: was 4 */

/* `n` travels with the array, because inside this function `in` is only
   the address of the array's first element: its length did not come with
   it (see broken-sizeof.c). */
static void
moving_average (const float in[], float out[], int n)
{
  for (int i = 0; i < n; i++)
    {
      float sum = 0.0f;
      int count = 0;
      /* Look back over the window, but never before the first sample. */
      for (int k = 0; k < WINDOW && i - k >= 0; k++)
        {
          sum += in[i - k];
          count++;
        }
      out[i] = sum / (float)count;
    }
}

int
main (void)
{
  /* A step: silence, then a constant level, then silence again. */
  float in[] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };

  /* HERE, in the scope that declared it, sizeof knows the whole array:
     total bytes / bytes per element = number of elements. */
  int n = (int)(sizeof in / sizeof in[0]);
  float out[sizeof in / sizeof in[0]];

  moving_average (in, out, n);
  printf ("%d samples, window %d\n", n, WINDOW);
  printf (" in :");
  for (int i = 0; i < n; i++)
    printf (" %5.0f", in[i]);
  printf ("\n out:");
  for (int i = 0; i < n; i++)
    printf (" %5.3f", out[i]);
  printf ("\n");
  return 0;
}
