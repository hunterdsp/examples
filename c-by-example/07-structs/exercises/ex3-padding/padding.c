/* padding.c (exercise 3) -- the same four members, in two orders.
 *
 * Each int must start at a multiple of 4, so after a char the compiler
 * inserts 3 bytes of padding. Interleaved, that happens twice, and the
 * struct grows to 16 bytes to carry 10 bytes of data. Put the ints first
 * and the two chars share one 4-byte slot at the end: 12 bytes.
 */
#include <stddef.h> /* offsetof */
#include <stdio.h>

struct interleaved
{
  char a; /* 0, then 3 bytes of padding */
  int  b; /* 4 */
  char c; /* 8, then 3 more */
  int  d; /* 12 */
};

struct sorted
{
  int  b; /* 0 */
  int  d; /* 4 */
  char a; /* 8 */
  char c; /* 9, then 2 bytes so an array of these keeps b aligned */
};

int
main (void)
{
  printf ("interleaved: sizeof %zu, offsets a %zu b %zu c %zu d %zu\n",
          sizeof (struct interleaved), offsetof (struct interleaved, a),
          offsetof (struct interleaved, b), offsetof (struct interleaved, c),
          offsetof (struct interleaved, d));
  printf ("sorted:      sizeof %zu, offsets b %zu d %zu a %zu c %zu\n",
          sizeof (struct sorted), offsetof (struct sorted, b),
          offsetof (struct sorted, d), offsetof (struct sorted, a),
          offsetof (struct sorted, c));
  return 0;
}
