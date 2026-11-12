/* limits.c (exercise 1) -- how long a counter of 48 kHz samples lasts.
 *
 * A sample counter that overflows is a bug that waits: it works for hours
 * and fails in production. Divide each type's largest value by the number
 * of samples in a day to see how long each one lasts.
 */
#include <inttypes.h> /* PRId32, PRId64: printf formats for exact types */
#include <stdint.h>
#include <stdio.h>

int
main (void)
{
  const double per_day = 48000.0 * 86400.0;

  printf ("INT32_MAX = %" PRId32 "\n", INT32_MAX);
  printf ("INT64_MAX = %" PRId64 "\n", INT64_MAX);
  printf ("int32_t lasts %.1f hours\n", INT32_MAX / per_day * 24.0);
  /* A double holds 53 bits of precision, so it cannot hold INT64_MAX
     (63 bits) exactly: the conversion rounds it up by one. Saying
     (double) makes that rounding deliberate, and clang stops warning. */
  printf ("int64_t lasts %.1f million years\n",
          (double)INT64_MAX / per_day / 365.25 / 1e6);
  return 0;
}
