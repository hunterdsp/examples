/* channels.c (exercise 3) -- several channels of samples in one block.
 *
 * C has no 2-D array whose size is decided at run time (lesson 04's
 * arrays are sized when compiling), so allocate channels * len floats in
 * one malloc and do the indexing yourself: channel ch's sample i lives
 * at ch * len + i. One allocation, one free, and every channel sits next
 * to the last in memory.
 */
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
  /* Both sizes are chosen when the program runs. */
  size_t channels = argc > 1 ? strtoul (argv[1], NULL, 10) : 4;
  size_t len      = argc > 2 ? strtoul (argv[2], NULL, 10) : 8;

  float *buf = malloc (channels * len * sizeof *buf);
  if (buf == NULL)
    return 1;

  /* Channel ch is a ramp at a different slope, so each row differs. */
  for (size_t ch = 0; ch < channels; ch++)
    for (size_t i = 0; i < len; i++)
      buf[ch * len + i] = ((float)ch + 1.0f) * (float)i;

  for (size_t ch = 0; ch < channels; ch++)
    {
      /* &buf[ch * len] is where channel ch starts: one row, len long. */
      const float *row = &buf[ch * len];
      printf ("ch %zu:", ch);
      for (size_t i = 0; i < len; i++)
        printf (" %4.0f", row[i]);
      printf ("\n");
    }
  printf ("%zu x %zu samples in one block of %zu bytes\n", channels, len,
          channels * len * sizeof *buf);
  free (buf);
  return 0;
}
