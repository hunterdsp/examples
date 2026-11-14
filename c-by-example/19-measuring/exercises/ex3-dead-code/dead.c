/* dead.c -- a benchmark the compiler deletes.
 *
 *   cc -std=c99 -Wall -Wextra -O2 dead.c -o dead && ./dead time
 *
 * Both loops do the same work. The first throws its result away, and at
 * -O2 the compiler, seeing that nothing uses it, does not run the loop at
 * all. The second stores its result in a volatile variable, which the
 * compiler must assume someone reads, so the work has to happen.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N    = 4800,
  REPS = 20000
};

static float x[N];

static float
sum (const float *v, size_t n)
{
  float s = 0.0f;
  for (size_t i = 0; i < n; i++)
    s += v[i];
  return s;
}

static volatile float sink;

int
main (int argc, char **argv)
{
  for (size_t i = 0; i < N; i++)
    x[i] = 0.001f;
  printf ("sum of %d samples of 0.001: %.3f\n", N, (double)sum (x, N));

  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      double  per   = 1e9 / ((double)REPS * (double)N);
      clock_t start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        (void)sum (x, N); /* result unused */
      double unused = (double)(clock () - start) / (double)CLOCKS_PER_SEC;

      start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        sink = sum (x, N); /* result kept */
      double kept = (double)(clock () - start) / (double)CLOCKS_PER_SEC;

      printf ("result unused: %5.2f ns per sample\n", unused * per);
      printf ("result kept:   %5.2f ns per sample\n", kept * per);
    }
  return 0;
}
