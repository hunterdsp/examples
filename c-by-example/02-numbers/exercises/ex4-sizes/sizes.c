/* sizes.c (exercise 4) -- how many bits a result needs, and the free
 * modulus of unsigned types.
 *
 * Each line checks one row of the lesson's table: the result is held in
 * a type big enough for it, and widened BEFORE the operation.
 */
#include <inttypes.h> /* PRId64, PRIu32: printf formats for exact types */
#include <stdint.h>
#include <stdio.h>

/* Bits a signed value needs: its magnitude's bits, plus one for the sign. */
static int
bits (int64_t v)
{
  int n = 1;
  for (uint64_t m = (uint64_t)(v < 0 ? -(v + 1) : v); m; m >>= 1)
    n++;
  return n;
}

int
main (void)
{
  /* a + b: N + 1 bits. Every operation below casts both operands
     itself rather than counting on C to convert them: the casts are free,
     and they say what the code means. */
  int16_t x = INT16_MAX, y = INT16_MAX;
  int32_t sum2 = (int32_t)x + (int32_t)y;
  printf ("int16_t + int16_t   = %" PRId32 " (%d bits)\n", sum2, bits (sum2));

  /* K values: N + ceil(log2 K) bits. 1024 = 2^10 adds 10 bits. */
  int32_t sum = 0;
  for (int k = 0; k < 1024; k++)
    sum += (int32_t)INT16_MAX;
  printf ("1024 int16_t summed = %" PRId32 " (%d bits)\n", sum, bits (sum));

  /* a * b: N + M bits. The most negative int16_t, squared, is the
     largest product. */
  int16_t lo = INT16_MIN;
  int32_t sq = (int32_t)lo * (int32_t)lo;
  printf ("int16_t * int16_t   = %" PRId32 " (%d bits)\n", sq, bits (sq));

  /* Two int32_t need 64 bits, so widen one operand first. */
  int32_t a = 100000, b = 100000;
  int64_t p = (int64_t)a * (int64_t)b;
  printf ("int32_t * int32_t   = %" PRId64 " (%d bits)\n", p, bits (p));

  /* Unsigned wraps modulo 2^32: a phase accumulator stepping an eighth of
     a cycle is back at 0 after 8 steps, with no if and no fmod. */
  uint32_t phase = 0, step = (uint32_t)1 << 29; /* 2^32 / 8 */
  for (int k = 0; k < 8; k++)
    phase += step;
  printf ("uint32_t phase after 8 steps of 2^32/8 = %" PRIu32 "\n", phase);
  return 0;
}
