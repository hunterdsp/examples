/* shift.c (exercise 2) -- the same mask, built unsigned.
 *
 * `(uint32_t)1 << bit` shifts an unsigned 32-bit 1, so every bit from 0
 * to 31 is a well-defined result. (Bit 32 and beyond is still undefined,
 * for any type: the shift must be less than the width.)
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
  (void)argv;
  int      bit  = 30 + argc; /* 31, as in broken-shift.c */
  uint32_t mask = (uint32_t)1 << bit;
  printf ("mask 0x%08" PRIx32 " (bit %d)\n", mask, bit);
  return 0;
}
