/* broken-sign-compare.c -- comparing signed with unsigned.
 *
 * In `i < n`, the int -1 is converted to unsigned before comparing, and
 * becomes 4294967295: so -1 < 1u is false. -Wextra catches the mix.
 */
#include <stdio.h>

int
main (void)
{
  int i = -1;
  unsigned n = 1;
  if (i < n)
    puts ("-1 is less than 1");
  else
    puts ("-1 is NOT less than 1?!");
  return 0;
}
