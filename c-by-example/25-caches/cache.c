/* cache.c -- the same work, in two memory orders.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       cache.c -o cache && ./cache
 *   cc -std=c99 -Wall -Wextra -O2 cache.c -o cache && ./cache time
 *
 * Memory is far slower than the processor, so the processor keeps copies
 * of recently used memory in CACHES, small and fast, and fetches memory
 * a whole CACHE LINE at a time (64 bytes on most machines: 16 floats).
 * Code that uses every byte of each line it fetches runs at cache speed.
 * Code that uses one float per line fetches sixteen times the memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
  ROWS   = 2048,
  COLS   = 2048, /* 2048 x 2048 floats: 16 MB, far bigger than any cache */
  VOICES = 100000
};

/* A synthesizer voice with everything about it in one struct: an ARRAY
   OF STRUCTS. The loop below only needs phase and inc. */
typedef struct
{
  float phase, inc; /* used every sample */
  float gain, pan;  /* used now and then */
  char  name[48];   /* used by the user interface */
} voice;            /* 64 bytes: one cache line per voice */

/* The same voices as a STRUCT OF ARRAYS: each field in its own array. */
typedef struct
{
  float *phase, *inc, *gain, *pan;
  char (*name)[48];
} voices;

static double
seconds_since (clock_t start)
{
  return (double)(clock () - start) / (double)CLOCKS_PER_SEC;
}

int
main (int argc, char **argv)
{
  int timing = argc > 1 && strcmp (argv[1], "time") == 0;

  /* 1. One 2-D array, summed along its rows and down its columns. C
        stores a 2-D array row after row, so a[r][c] and a[r][c + 1] are
        neighbours in memory, and a[r][c] and a[r + 1][c] are 8 KB apart. */
  float (*a)[COLS] = malloc (sizeof (float[ROWS][COLS]));
  if (a == NULL)
    return 1;
  for (size_t r = 0; r < ROWS; r++)
    for (size_t c = 0; c < COLS; c++)
      a[r][c] = (float)((r + c) % (size_t)4); /* small whole numbers: exact */

  clock_t start  = clock ();
  double  by_row = 0.0;
  for (size_t r = 0; r < ROWS; r++)
    for (size_t c = 0; c < COLS; c++)
      by_row += (double)a[r][c];
  double t_row = seconds_since (start);

  start         = clock ();
  double by_col = 0.0;
  for (size_t c = 0; c < COLS; c++)
    for (size_t r = 0; r < ROWS; r++)
      by_col += (double)a[r][c];
  double t_col = seconds_since (start);
  free (a);
  printf ("sum by rows %.0f, by columns %.0f\n", by_row, by_col);

  /* 2. Advancing every voice's phase: array of structs, struct of
        arrays. */
  voice *aos = calloc (VOICES, sizeof *aos);
  voices soa = {
    calloc (VOICES, sizeof (float)),   calloc (VOICES, sizeof (float)),
    calloc (VOICES, sizeof (float)),   calloc (VOICES, sizeof (float)),
    calloc (VOICES, sizeof *soa.name),
  };
  if (aos == NULL || !soa.phase || !soa.inc || !soa.gain || !soa.pan
      || !soa.name)
    return 1;
  for (size_t v = 0; v < VOICES; v++)
    aos[v].inc = soa.inc[v] = 0.001f * (float)(v % (size_t)100);

  start = clock ();
  for (int step = 0; step < 100; step++)
    for (size_t v = 0; v < VOICES; v++)
      aos[v].phase += aos[v].inc;
  double t_aos = seconds_since (start);

  start = clock ();
  for (int step = 0; step < 100; step++)
    for (size_t v = 0; v < VOICES; v++)
      soa.phase[v] += soa.inc[v];
  double t_soa = seconds_since (start);

  int same = 1;
  for (size_t v = 0; v < VOICES; v++)
    same = same && aos[v].phase == soa.phase[v];
  printf ("%zu-byte voices: both layouts agree: %s\n", sizeof (voice),
          same ? "yes" : "no");

  if (timing)
    {
      printf ("sum by rows:     %6.1f ms\n", t_row * 1e3);
      printf ("sum by columns:  %6.1f ms\n", t_col * 1e3);
      printf ("array of structs: %5.1f ms\n", t_aos * 1e3);
      printf ("struct of arrays: %5.1f ms\n", t_soa * 1e3);
    }
  free (aos);
  free (soa.phase);
  free (soa.inc);
  free (soa.gain);
  free (soa.pan);
  free (soa.name);
  return 0;
}
