/* gain.c -- a loop for the compiler to vectorise, twice.
 *
 *   cc -std=c99 -Wall -Wextra -O2 -c gain.c
 *
 * Built with a vectoriser report turned on (see the README), the
 * compiler says which loops it vectorised, and how.
 */
#include <stddef.h>

/* out and in might overlap: the compiler has to allow for it. */
void
gain (float *out, const float *in, float g, size_t n)
{
  for (size_t i = 0; i < n; i++)
    out[i] = g * in[i];
}

/* restrict: the caller promises they don't, so the compiler needn't
   check. */
void
gain_restrict (float *restrict out, const float *restrict in, float g,
               size_t n)
{
  for (size_t i = 0; i < n; i++)
    out[i] = g * in[i];
}
