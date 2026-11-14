/* median.c -- qsort, the standard library's own function pointer.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       median.c -o median && ./median
 *
 * qsort sorts an array of anything, so it can't compare the elements
 * itself: the caller passes a function that compares two of them. It
 * hands that function two const void * (it doesn't know the type), and
 * the comparator turns them back into what they are.
 */
#include <stdio.h>
#include <stdlib.h>

/* Negative if *a sorts first, positive if *b does, 0 if equal. */
static int
cmp_float (const void *a, const void *b)
{
  float x = *(const float *)a;
  float y = *(const float *)b;
  return (x > y) - (x < y); /* each comparison is 0 or 1 */
}

/* The same, written the tempting way: x - y, converted to int. */
static int
cmp_float_wrong (const void *a, const void *b)
{
  float x = *(const float *)a;
  float y = *(const float *)b;
  return (int)(x - y); /* 0.05 becomes 0: "equal" */
}

enum
{
  N = 7
};

static void
show (const char *label, const float *buf)
{
  printf ("%-8s", label);
  for (size_t i = 0; i < N; i++)
    printf (" %.2f", (double)buf[i]);
  printf ("\n");
}

int
main (void)
{
  /* Quiet samples, and one click. */
  const float block[N] = { 0.10f, 0.20f, 0.15f, 9.00f, 0.20f, 0.10f, 0.25f };

  double sum = 0.0;
  for (size_t i = 0; i < N; i++)
    sum += (double)block[i];

  float sorted[N];
  for (size_t i = 0; i < N; i++)
    sorted[i] = block[i];
  qsort (sorted, N, sizeof sorted[0], cmp_float);
  show ("sorted:", sorted);
  printf ("mean %.2f, median %.2f\n", sum / (double)N, (double)sorted[N / 2]);

  /* What the tempting comparator says about two different samples. */
  printf ("x - y compares 0.10 with 0.15: %d, \"equal\"\n",
          cmp_float_wrong (&block[0], &block[2]));
  return 0;
}
