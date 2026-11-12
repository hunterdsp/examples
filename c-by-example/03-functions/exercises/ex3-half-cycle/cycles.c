/* cycles.c (exercise 3) -- 3.5 cycles in 8 samples, played on repeat.
 *
 * Built against the lesson's own sine.c (see the README for the command).
 *
 * `wave` is the sine carried on for 16 samples: 7 cycles over 16 is the
 * same frequency as 3.5 over 8. `looped` is the 8-sample table played
 * twice, which is what a loop of it sounds like. They agree for the first
 * copy, and from the seam on the loop is the wave upside down.
 */
#include <stdio.h>

#include "sine.h"

static void
print_row (const char *label, const float row[], size_t n)
{
  printf ("%s:", label);
  for (size_t i = 0; i < n; i++)
    printf (" %5.2f", row[i]);
  printf ("\n");
}

int
main (void)
{
  float table[8], looped[16], wave[16];
  sine_table (table, 8, 3.5);
  for (size_t i = 0; i < 16; i++)
    looped[i] = table[i % 8];
  sine_table (wave, 16, 7.0);
  print_row ("wave  ", wave, 16);
  print_row ("looped", looped, 16);
  return 0;
}
