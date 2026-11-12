/* decibels.c (exercise 2) -- decibels back to amplitude ratios.
 *
 * Inverting 20 log10(ratio) gives ratio = 10^(dB / 20).
 */
#include <math.h>
#include <stdio.h>

int
main (void)
{
  const double levels[] = { -20.0, -40.0, -60.0 };
  for (int i = 0; i < 3; i++)
    printf ("%3.0f dB = %.4f x amplitude\n", levels[i],
            pow (10.0, levels[i] / 20.0));
  return 0;
}
