/* stride.c -- reading every k-th int, for k = 1, 2, 4, ... 64.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       stride.c -o stride && ./stride
 *   cc -std=c99 -Wall -Wextra -O2 stride.c -o stride && ./stride time
 *
 * The array is 64 MB, far bigger than the caches, so every cache line it
 * touches comes from main memory. Reading every k-th int reads 1/k of
 * them; how much of the TIME does it save?
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
  N = 16 * 1024 * 1024 /* ints: 64 MB */
};

int
main (int argc, char **argv)
{
  int  timing = argc > 1 && strcmp (argv[1], "time") == 0;
  int *a      = malloc ((size_t)N * sizeof *a);
  if (a == NULL)
    return 1;
  for (size_t i = 0; i < N; i++)
    a[i] = 1;

  printf ("stride    ints read  bytes apart");
  printf (timing ? "  ms total  ns per int\n" : "\n");
  for (size_t k = 1; k <= 64; k *= 2)
    {
      clock_t start = clock ();
      long    sum   = 0;
      for (size_t i = 0; i < N; i += k)
        sum += a[i];
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      printf ("%6zu  %11ld  %11zu", k, sum, k * sizeof (int));
      if (timing)
        printf ("  %8.1f  %10.2f", s * 1e3, s * 1e9 / (double)sum);
      printf ("\n");
    }
  free (a);
  return 0;
}
