/* sums.c -- the eight-lane sum, accumulated in float and in double.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       sums.c -o sums && ./sums
 *   cc -std=c99 -Wall -Wextra -O2 sums.c -o sums && ./sums time
 *
 * A double has 53 bits of precision to a float's 24, so its running
 * totals round far less. A vector register holds half as many of them,
 * though. Is accuracy expensive?
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N     = 1 << 20,
  LANES = 8,
  REPS  = 100
};

static float x[N];

static double
lanes_float (const float *v, size_t n)
{
  float lane[LANES] = { 0.0f };
  for (size_t i = 0; i + LANES <= n; i += LANES)
    for (size_t k = 0; k < LANES; k++)
      lane[k] += v[i + k];
  float s = 0.0f;
  for (size_t k = 0; k < LANES; k++)
    s += lane[k];
  return (double)s;
}

static double
lanes_double (const float *v, size_t n)
{
  double lane[LANES] = { 0.0 };
  for (size_t i = 0; i + LANES <= n; i += LANES)
    for (size_t k = 0; k < LANES; k++)
      lane[k] += (double)v[i + k];
  double s = 0.0;
  for (size_t k = 0; k < LANES; k++)
    s += lane[k];
  return s;
}

typedef double (*sum_fn) (const float *v, size_t n);

static volatile double sink;

static double
best_ms (sum_fn f)
{
  double best = 1e30;
  for (int run = 0; run < 5; run++)
    {
      clock_t start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        sink = f (x, N);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      best     = s < best ? s : best;
    }
  return best * 1e3 / (double)REPS;
}

int
main (int argc, char **argv)
{
  for (size_t i = 0; i < N; i++)
    x[i] = 0.1f;
  printf ("float lanes:  %.4f\n", lanes_float (x, N));
  printf ("double lanes: %.4f\n", lanes_double (x, N));
  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      printf ("float lanes:  %6.3f ms\n", best_ms (lanes_float));
      printf ("double lanes: %6.3f ms\n", best_ms (lanes_double));
    }
  return 0;
}
