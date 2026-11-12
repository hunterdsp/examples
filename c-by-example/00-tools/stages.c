/* stages.c -- the smallest program worth taking apart.
 *
 * Build it in one step:      cc stages.c -o stages && ./stages
 * Or watch each stage:       see README.md
 */
#include <stdio.h>

#define GREETING "compiled, linked, ran"

int
main (void)
{
  puts (GREETING);
  return 0;
}
