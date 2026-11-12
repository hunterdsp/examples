/* pointers.c -- an address is a value you can pass around.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       pointers.c -o pointers && ./pointers
 *
 * A pointer holds the address of something. `&x` is x's address; `*p` is
 * whatever p points at. That is the whole idea, and everything in Parts
 * II and III is built on it.
 */
#include <stddef.h>
#include <stdio.h>

/* Lesson 03 could not change the caller's variable. With its ADDRESS, a
   function can: it writes where the variable lives. */
static void
swap (float *a, float *b)
{
  float tmp = *a;
  *a = *b;
  *b = tmp;
}

/* In place: read and write the same buffer. `buf` is a pointer to the
   first sample, and `n` is how many follow (an array arrives as exactly
   this, lesson 04). */
static void
gain_in_place (float *buf, size_t n, float g)
{
  for (size_t i = 0; i < n; i++)
    buf[i] *= g; /* buf[i] is shorthand for *(buf + i) */
}

/* Out of place, with the contract in the types:
     const float *in   -- this function will not write the input
     float *out        -- it will write the output
     restrict          -- in and out never overlap, so the compiler may
                          keep values in registers (lesson 26) */
static void
gain (const float *restrict in, float *restrict out, size_t n, float g)
{
  for (size_t i = 0; i < n; i++)
    out[i] = in[i] * g;
}

/* Pointer arithmetic: p + 1 is the NEXT float, 4 bytes on, not 1 byte.
   A loop can walk a pointer instead of an index. Returns NULL (a pointer
   to nothing) when there is no peak to report. */
static const float *
find_peak (const float *buf, size_t n)
{
  if (n == 0)
    return NULL;
  const float *peak = buf;
  for (const float *p = buf + 1; p < buf + n; p++)
    if (*p > *peak)
      peak = p;
  return peak;
}

int
main (void)
{
  float x = 1.0f, y = 2.0f;
  swap (&x, &y);
  printf ("after swap: x = %.1f, y = %.1f\n", x, y);

  float sig[] = { 0.5f, -1.0f, 2.0f, 0.3f };
  size_t n = sizeof sig / sizeof sig[0];

  float half[4];
  gain (sig, half, n, 0.5f);
  gain_in_place (sig, n, 2.0f);
  printf ("doubled:");
  for (size_t i = 0; i < n; i++)
    printf (" %.2f", sig[i]);
  printf ("\nhalved: ");
  for (size_t i = 0; i < n; i++)
    printf (" %.2f", half[i]);
  printf ("\n");

  const float *peak = find_peak (sig, n);
  /* Subtracting two pointers into one array gives the distance between
     them, in elements: here, the peak's index. */
  printf ("peak %.2f at index %td\n", *peak, peak - sig);

  if (find_peak (sig, 0) == NULL)
    printf ("empty buffer: no peak (NULL)\n");

  printf ("sig[1] lives %td bytes after sig[0]\n",
          (const char *)&sig[1] - (const char *)&sig[0]);
  return 0;
}
