/* vec.c -- one sum, added up in two orders.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       vec.c -o vec && ./vec
 *   cc -std=c99 -Wall -Wextra -O2 vec.c -o vec && ./vec time
 *
 * A processor's VECTOR instructions (SSE and AVX on x86, NEON on ARM)
 * work on several floats at once: 4, 8 or 16 in one instruction. The
 * compiler uses them for a loop when it can prove the result is the
 * same. A float sum is the classic loop it can NOT: adding in a
 * different order rounds differently, and so gives a different answer.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  N     = 1 << 20, /* 1048576 samples */
  LANES = 8,
  REPS  = 100
};

static float x[N];

/* One running total: each addition waits for the one before. */
static float
sum_in_order (const float *v, size_t n)
{
  float s = 0.0f;
  for (size_t i = 0; i < n; i++)
    s += v[i];
  return s;
}

/* Eight running totals, side by side, combined at the end. A DIFFERENT
   order of additions, written out, so the compiler is free to put the
   eight lanes in one vector register and add eight at a time. */
static float
sum_in_lanes (const float *v, size_t n)
{
  float  lane[LANES] = { 0.0f };
  size_t i           = 0;
  for (; i + LANES <= n; i += LANES)
    for (size_t k = 0; k < LANES; k++)
      lane[k] += v[i + k];
  for (; i < n; i++) /* the last n % 8, if any */
    lane[0] += v[i];
  for (size_t width = LANES / 2; width > 0; width /= 2) /* 8 -> 4 -> 2 -> 1 */
    for (size_t k = 0; k < width; k++)
      lane[k] += lane[k + width];
  return lane[0];
}

typedef float (*sum_fn) (const float *v, size_t n);

static volatile float sink;

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
  double exact = 0.0;
  for (size_t i = 0; i < N; i++)
    {
      x[i] = 0.1f;
      exact += (double)x[i]; /* double: as good as exact here */
    }
  printf ("%d samples of 0.1f\n", N);
  printf ("in order:  %.4f\n", (double)sum_in_order (x, N));
  printf ("in lanes:  %.4f\n", (double)sum_in_lanes (x, N));
  printf ("in double: %.4f\n", exact);

  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      printf ("in order: %6.3f ms\n", best_ms (sum_in_order));
      printf ("in lanes: %6.3f ms\n", best_ms (sum_in_lanes));
    }
  return 0;
}
