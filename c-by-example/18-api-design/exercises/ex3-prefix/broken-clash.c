/* broken-clash.c -- two libraries, one name.
 *
 * Each header is fine alone. Together, both define a type called filter
 * and declare a function called process, and C has a single namespace
 * for them: the program can't use both libraries at all. Prefixes
 * (fir_filter, smooth_process) would have kept them apart.
 */
#include "fir.h"
#include "smooth.h"

int
main (void)
{
  return 0;
}
