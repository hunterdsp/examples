/* transpose.c -- swapping rows and columns, plainly and in tiles.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       transpose.c -o transpose && ./transpose
 *   cc -std=c99 -Wall -Wextra -O2 transpose.c -o transpose && ./transpose time
 *
 * out[c][r] = in[r][c] reads `in` along its rows, which is good, and
 * writes `out` down its columns, which is lesson 25's slow direction. No
 * loop order fixes that: one of the two is always crossways. Working in
 * square TILES keeps a tile of each small enough to stay in the cache
 * while all of it is used. The best tile size depends on the machine,
 * so this tries several.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
  N = 2048 /* a multiple of every tile size below */
};

/* tile = 1 is the plain double loop. */
static void
transpose (float (*out)[N], float (*in)[N], size_t tile)
{
  for (size_t r0 = 0; r0 < N; r0 += tile)
    for (size_t c0 = 0; c0 < N; c0 += tile)
      for (size_t r = r0; r < r0 + tile; r++)
        for (size_t c = c0; c < c0 + tile; c++)
          out[c][r] = in[r][c];
}

int
main (int argc, char **argv)
{
  int timing      = argc > 1 && strcmp (argv[1], "time") == 0;
  float (*in)[N]  = malloc (sizeof (float[N][N]));
  float (*ref)[N] = malloc (sizeof (float[N][N]));
  float (*out)[N] = malloc (sizeof (float[N][N]));
  if (in == NULL || ref == NULL || out == NULL)
    return 1;
  for (size_t r = 0; r < N; r++)
    for (size_t c = 0; c < N; c++)
      in[r][c] = (float)(r * N + c);
  transpose (ref, in, 1);

  const size_t tiles[] = { 1, 8, 16, 32, 64 };
  for (size_t t = 0; t < sizeof tiles / sizeof tiles[0]; t++)
    {
      memset (out, 0, sizeof (float[N][N]));
      clock_t start = clock ();
      transpose (out, in, tiles[t]);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      printf ("tile %2zu: same as plain: %s", tiles[t],
              memcmp (out, ref, sizeof (float[N][N])) == 0 ? "yes" : "no");
      if (timing)
        printf (", %5.1f ms", s * 1e3);
      printf ("\n");
    }
  free (in);
  free (ref);
  free (out);
  return 0;
}
