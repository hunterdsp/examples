/* pow.c -- pow(x, 2.0) or x * x: is the "obvious" speed-up real?
 *
 *   cc -std=c99 -Wall -Wextra -O0 pow.c -o pow -lm && ./pow time
 *   cc -std=c99 -Wall -Wextra -O2 pow.c -o pow -lm && ./pow time
 *
 * pow is a general function, for any power, so squaring with it looks
 * wasteful. Measure it, at both optimisation levels, before rewriting.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N    = 4800,
  RUNS = 7,
  REPS = 200
};

static float x[N];

static double
energy_pow (const float *v, size_t n)
{
  double e = 0.0;
  for (size_t i = 0; i < n; i++)
    e += pow ((double)v[i], 2.0);
  return e;
}

static double
energy_mul (const float *v, size_t n)
{
  double e = 0.0;
  for (size_t i = 0; i < n; i++)
    e += (double)v[i] * (double)v[i];
  return e;
}

typedef double (*energy_fn) (const float *v, size_t n);

static volatile double sink; /* results go here, so none can be skipped */

static double
best_seconds (energy_fn f)
{
  double best = 1e30;
  for (int run = 0; run < RUNS; run++)
    {
      clock_t start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        sink = f (x, N);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      if (s < best)
        best = s;
    }
  return best;
}

int
main (int argc, char **argv)
{
  for (size_t i = 0; i < N; i++)
    x[i] = (float)sin (6.283185307179586 * 440.0 * (double)i / 48000.0);

  printf ("same energy: %s\n",
          energy_pow (x, N) == energy_mul (x, N) ? "yes" : "no");
  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      double per = 1e9 / ((double)REPS * (double)N);
      printf ("pow:    %5.2f ns per sample\n",
              best_seconds (energy_pow) * per);
      printf ("x * x:  %5.2f ns per sample\n",
              best_seconds (energy_mul) * per);
    }
  return 0;
}
