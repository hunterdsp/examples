/* broken-overflow.c -- signed overflow is undefined behaviour.
 *
 * 48000 * 86400 is about 4.1 billion; an int32_t stops at 2.1 billion.
 * Unsigned types wrap, but for signed ones the C standard says the
 * behaviour is undefined: the compiler may assume it never happens and
 * optimise accordingly. UBSan (-fsanitize=undefined) stops the program.
 */
#include <stdint.h>
#include <stdio.h>

static int32_t
samples_per_day (int32_t rate)
{
  return rate * 86400; /* overflows for any rate above 24855 */
}

int
main (int argc, char *argv[])
{
  (void)argv;
  /* argc keeps the compiler from working the answer out at build time. */
  printf ("%d\n", samples_per_day (48000 + argc - 1));
  return 0;
}
