/* broken-lane-tail.c -- eight at a time, with no care for the end.
 *
 * Processing in lanes of 8 means the loop's last step must not go past
 * n. Written `i < n` instead of `i + 8 <= n`, the last step reads up to
 * 7 floats past the end whenever n isn't a multiple of 8: here n is
 * 1003, and the sanitizers catch the read at index 1003.
 */
#include <stdio.h>
#include <stdlib.h>

enum
{
  LANES = 8
};

int
main (void)
{
  size_t n = 1003;
  float *x = malloc (n * sizeof *x);
  if (x == NULL)
    return 1;
  for (size_t i = 0; i < n; i++)
    x[i] = 0.1f;

  float lane[LANES] = { 0.0f };
  for (size_t i = 0; i < n; i += LANES) /* should be i + LANES <= n */
    for (size_t k = 0; k < LANES; k++)
      lane[k] += x[i + k];

  float sum = 0.0f;
  for (size_t k = 0; k < LANES; k++)
    sum += lane[k];
  printf ("%.2f\n", (double)sum);
  free (x);
  return 0;
}
