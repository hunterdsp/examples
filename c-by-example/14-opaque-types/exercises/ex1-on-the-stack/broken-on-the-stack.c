/* broken-on-the-stack.c -- a lowpass_t as a local variable.
 *
 * To make room for `lowpass_t f;` on the stack, the compiler has to know
 * how big a lowpass_t is, and outside lowpass.c it doesn't: the header
 * only says the struct exists. Neither the variable nor sizeof compiles.
 */
#include "lowpass.h"

#include <stdio.h>

int
main (void)
{
  lowpass_t f; /* how many bytes? */
  printf ("%zu\n", sizeof (lowpass_t));
  (void)f;
  return 0;
}
