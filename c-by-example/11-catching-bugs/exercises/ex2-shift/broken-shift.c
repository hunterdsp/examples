/* broken-shift.c (exercise 2) -- shifting a 1 into an int's sign bit.
 *
 * Building a 32-bit mask with `1 << bit` works for bits 0 to 30. At bit
 * 31 the result, 2,147,483,648, does not fit in an int, and in C99 that
 * shift is undefined behaviour, like any signed overflow.
 * UndefinedBehaviorSanitizer stops the program at it.
 */
#include <stdio.h>

int
main (int argc, char *argv[])
{
  (void)argv;
  int bit  = 30 + argc; /* 31, but decided at run time */
  int mask = 1 << bit;  /* 1 is an int: the shift is signed */
  printf ("mask %d\n", mask);
  return 0;
}
