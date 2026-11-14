/* doubled.c -- the FIR inner loop without the wrap-around.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       doubled.c -o doubled -lm && ./doubled
 *   cc -std=c99 -Wall -Wextra -O2 doubled.c -o doubled -lm && ./doubled time
 *
 * fir.c's inner loop steps back through a ring, checking at every tap
 * whether to wrap. Here every input is written TWICE, n slots apart, in
 * a history 2n long. Then the last n inputs always sit side by side,
 * oldest first, at hist + newest + 1, and the inner loop is a plain dot
 * product: no wrap, no branch, and easy for the compiler to speed up.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  TAPS = 63,
  N    = 4800,
  RUNS = 7,
  REPS = 50
};

static float h[TAPS];
static float x[N], y_ring[N], y_doubled[N];

/* fir.c's way: a ring of TAPS, newest first, wrapping. */
static void
by_ring (const float *in, float *out, size_t n)
{
  float  hist[TAPS] = { 0.0f };
  size_t newest     = 0;
  for (size_t i = 0; i < n; i++)
    {
      newest       = newest + 1 < TAPS ? newest + 1 : 0;
      hist[newest] = in[i];
      float  acc   = 0.0f;
      size_t j     = newest;
      for (size_t k = 0; k < TAPS; k++)
        {
          acc += h[k] * hist[j];
          j = j > 0 ? j - 1 : TAPS - 1;
        }
      out[i] = acc;
    }
}

/* Each input stored at slot s and s + TAPS. The newest is at
   newest + TAPS, the oldest at newest + 1: TAPS floats in a row. */
static void
by_doubled (const float *in, float *out, size_t n)
{
  float  hist[2 * TAPS] = { 0.0f };
  size_t newest         = 0;
  for (size_t i = 0; i < n; i++)
    {
      newest              = newest + 1 < TAPS ? newest + 1 : 0;
      hist[newest]        = in[i];
      hist[newest + TAPS] = in[i];
      const float *past   = hist + newest + 1; /* oldest first */
      float        acc    = 0.0f;
      for (size_t k = 0; k < TAPS; k++)
        acc += h[TAPS - 1 - k] * past[k]; /* oldest meets the last tap */
      out[i] = acc;
    }
}

typedef void (*filter_fn) (const float *in, float *out, size_t n);

static double
best_seconds (filter_fn f, float *out)
{
  double best = 1e30;
  for (int run = 0; run < RUNS; run++)
    {
      clock_t start = clock ();
      for (int rep = 0; rep < REPS; rep++)
        f (x, out, N);
      double s = (double)(clock () - start) / (double)CLOCKS_PER_SEC;
      best     = s < best ? s : best;
    }
  return best;
}

int
main (int argc, char **argv)
{
  for (size_t k = 0; k < TAPS; k++)
    h[k] = 1.0f / (float)TAPS;
  for (size_t i = 0; i < N; i++)
    x[i] = (float)sin (6.283185307179586 * 440.0 * (double)i / 48000.0);

  by_ring (x, y_ring, N);
  by_doubled (x, y_doubled, N);
  double worst = 0.0;
  for (size_t i = 0; i < N; i++)
    {
      double d = fabs ((double)y_ring[i] - (double)y_doubled[i]);
      worst    = d > worst ? d : worst;
    }
  printf ("%d taps, %d samples: same output (within 1e-6): %s\n", TAPS, N,
          worst < 1e-6 ? "yes" : "no");

  if (argc > 1 && strcmp (argv[1], "time") == 0)
    {
      double per = 1e9 / ((double)REPS * (double)N);
      printf ("ring:    %5.2f ns per sample\n",
              best_seconds (by_ring, y_ring) * per);
      printf ("doubled: %5.2f ns per sample\n",
              best_seconds (by_doubled, y_doubled) * per);
    }
  return 0;
}
