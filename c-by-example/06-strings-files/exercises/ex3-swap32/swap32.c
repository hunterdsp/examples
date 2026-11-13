/* swap32.c (exercise 3) -- reverse the byte order of a 32-bit value.
 *
 * Shift each byte to its mirror position and mask off the rest: byte 0
 * goes to byte 3, byte 1 to byte 2, and so on. This is what reading a
 * big-endian file on a little-endian machine needs (lesson 06's "Bytes").
 */
#include <inttypes.h> /* PRIx32: the printf format for uint32_t in hex */
#include <stdint.h>
#include <stdio.h>

static uint32_t
swap32 (uint32_t x)
{
  return (x >> 24)                  /* byte 3 -> byte 0 */
         | ((x >> 8) & 0x0000ff00u) /* byte 2 -> byte 1 */
         | ((x << 8) & 0x00ff0000u) /* byte 1 -> byte 2 */
         | (x << 24);               /* byte 0 -> byte 3 */
}

int
main (void)
{
  uint32_t one = 0x3f800000u; /* the bits of 1.0f, lesson 06 */
  printf ("swap32(0x%08" PRIx32 ") = 0x%08" PRIx32 "\n", one, swap32 (one));
  printf ("swap32(swap32(0x%08" PRIx32 ")) = 0x%08" PRIx32 "\n", one,
          swap32 (swap32 (one)));
  return 0;
}
