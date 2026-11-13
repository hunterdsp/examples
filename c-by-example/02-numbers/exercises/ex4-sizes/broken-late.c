/* broken-late.c (exercise 4) -- widening AFTER the multiply is too late.
 *
 * (int64_t)(a * b) multiplies two int32_t in 32 bits, overflows, and only
 * then widens the wrong answer. Signed overflow is undefined behaviour,
 * so UBSan stops the program. The fix is (int64_t)a * b: see sizes.c.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
  (void)argv;
  /* argc keeps the compiler from working the answer out at build time. */
  int32_t a = 100000 + argc - 1, b = 100000;
  int64_t p = (int64_t)(a * b);
  printf ("%" PRId64 "\n", p);
  return 0;
}
