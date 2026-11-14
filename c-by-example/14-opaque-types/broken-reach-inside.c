/* broken-reach-inside.c -- reaching inside an opaque type.
 *
 * lowpass.h declares `struct lowpass` but never defines it, so outside
 * lowpass.c the compiler does not know the struct has a member `a`, or
 * any members at all. Setting the coefficient directly does not compile:
 * the way in is through the functions the header declares.
 */
#include "lowpass.h"

int
main (void)
{
  lowpass_t *f = lowpass_create (0.5f);
  if (f == NULL)
    return 1;
  f->a = 0.9f; /* "just this once": the compiler says no */
  lowpass_destroy (f);
  return 0;
}
