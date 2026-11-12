/* halve.c (exercise 1) -- a table, then the same table at half amplitude.
 *
 * Half the amplitude is -6 dB (lesson 02).
 */
#include <stdio.h>

#include "sine.h"

static void
print_table (const char *label, const float table[], size_t n)
{
  printf ("%s:", label);
  for (size_t i = 0; i < n; i++)
    printf (" %7.4f", table[i]);
  printf ("\n");
}

int
main (void)
{
  float t[8];
  sine_table (t, 8, 1.0);
  print_table ("full", t, 8);
  sine_scale (t, 8, 0.5f);
  print_table ("half", t, 8);
  return 0;
}
