/* bench.c -- two ways to make a sine wave, checked, then timed.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       bench.c -o bench -lm && ./bench
 *   cc -std=c99 -Wall -Wextra -O2 bench.c -o bench -lm && ./bench time
 *
 * One way calls sin() for every sample. The other looks the sine up in a
 * table of 1024 values, made once, and interpolates between neighbours
 * (lesson 03's table, put to work). Run plainly, the program checks that
 * the two agree. Run with `time`, it also measures each one, which is
 * only meaningful in an optimised build without sanitizers.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N     = 4800, /* 0.1 s at 48 kHz */
  TABLE = 1024, /* a power of two: wrap with a mask (lesson 12) */
  RUNS  = 7,    /* time this many times, keep the fastest */
  REPS  = 200   /* calls per timing: long enough for clock() to see */
};

static const double TWO_PI = 6.283185307179586;
static const double FREQ   = 440.0 / 48000.0; /* cycles per sample */

static float table[TABLE + 1]; /* one extra, a copy of [0], for [i + 1] */
static float out[N];

static void
by_sin (float *y, size_t n)
{
  for (size_t i = 0; i < n; i++)
    y[i] = (float)sin (TWO_PI * FREQ * (double)i);
}

static void
by_table (float *y, size_t n)
{
  for (size_t i = 0; i < n; i++)
    {
      double pos  = FREQ * (double)i * (double)TABLE; /* in table steps */
      double base = floor (pos);
      size_t k    = (size_t)base & (size_t)(TABLE - 1);
      float  frac = (float)(pos - base);
      y[i]        = table[k] + frac * (table[k + 1] - table[k]);
    }
}

typedef void (*gen_fn) (float *y, size_t n);

/* CPU seconds for REPS calls of f, the fastest of RUNS tries. The first
   tries also warm up the caches; the minimum is the least disturbed by
   whatever else the machine was doing. */
static double
best_seconds (gen_fn f)
{
  double best = 1e30;
  for (int run = 0; run < RUNS; run++)
    {
      clock_t start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        f (out, N);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      if (s < best)
        best = s;
    }
  return best;
}

int
main (int argc, char **argv)
{
  for (size_t k = 0; k <= TABLE; k++)
    table[k] = (float)sin (TWO_PI * (double)k / (double)TABLE);

  /* First: are they the same wave? A fast wrong answer is no use. */
  static float a[N], b[N];
  by_sin (a, N);
  by_table (b, N);
  double worst = 0.0;
  for (size_t i = 0; i < N; i++)
    {
      double d = fabs ((double)a[i] - (double)b[i]);
      if (d > worst)
        worst = d;
    }
  printf ("%d samples, largest difference below 1e-5: %s\n", N,
          worst < 1e-5 ? "yes" : "no");

  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      double t_sin   = best_seconds (by_sin);
      double t_table = best_seconds (by_table);
      double per     = 1e9 / ((double)REPS * (double)N);
      printf ("sin():  %5.2f ns per sample\n", t_sin * per);
      printf ("table:  %5.2f ns per sample\n", t_table * per);
      printf ("table is %.1f times as fast\n", t_sin / t_table);
      /* Use the output, so the compiler can't decide it was never needed
         and skip making it (exercise 3). */
      printf ("(last sample %.4f)\n", (double)out[N - 1]);
    }
  return 0;
}
