/* broken-signed-overflow.c -- a sum too big for its type.
 *
 * 70,000 full-scale 16-bit samples add up to about 2.3 billion, more than
 * an int32_t holds (2,147,483,647). Signed overflow is undefined
 * behaviour (lesson 02): the compiler may assume it never happens, and
 * optimise as if it can't. UndefinedBehaviorSanitizer stops the program
 * at the addition that overflows.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
  size_t   n = 70000;
  int16_t *s = malloc (n * sizeof *s);
  if (s == NULL)
    return 1;
  for (size_t i = 0; i < n; i++)
    s[i] = INT16_MAX; /* full scale, every sample */

  int32_t sum = 0; /* lesson 02's table: 16 bits + log2(70000) needs 33 */
  for (size_t i = 0; i < n; i++)
    sum = (int32_t)sum + (int32_t)s[i];
  printf ("sum %" PRId32 "\n", sum);
  free (s);
  return 0;
}
