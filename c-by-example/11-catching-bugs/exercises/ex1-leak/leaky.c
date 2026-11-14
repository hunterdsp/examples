/* leaky.c (exercise 1) -- four blocks allocated, none freed.
 *
 * Each pass of the loop allocates a block, and the only copy of its
 * address is `block`, which goes out of scope at the closing brace. After
 * that nothing can free it: the memory is lost until the program exits.
 * LeakSanitizer (part of AddressSanitizer on Linux) reports every such
 * block when the program exits, with the line that allocated it.
 */
#include <stdio.h>
#include <stdlib.h>

/* Returns n zeroed floats the caller must free, or NULL. */
static float *
block_new (size_t n)
{
  return calloc (n, sizeof (float));
}

int
main (void)
{
  for (int i = 0; i < 4; i++)
    {
      float *block = block_new (256);
      if (block == NULL)
        return 1;
      printf ("block %d: %zu bytes\n", i, (size_t)256 * sizeof *block);
    } /* `block` ends here, and its address with it: a leak */
  return 0;
}
