/* broken-format.c (exercise 3) -- %d handed a string.
 *
 * printf cannot see its arguments' types: the format string is its only
 * guide, so a mismatch reads the wrong bytes. -Wall makes the compiler
 * check every format against its argument (-Wformat).
 */
#include <stdio.h>

int
main (void)
{
  printf ("%d\n", "three");
  return 0;
}
