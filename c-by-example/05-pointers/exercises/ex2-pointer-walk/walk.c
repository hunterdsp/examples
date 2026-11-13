/* walk.c (exercise 2) -- gain_in_place, walking a pointer.
 *
 * Instead of counting i from 0 to n and reading buf[i], move a pointer
 * from the first sample to one past the last. `buf + n` points just past
 * the end: C lets you form that address and compare against it, but never
 * read through it.
 */
#include <stddef.h>
#include <stdio.h>
#include <string.h> /* memcmp */

/* The lesson's version, kept to compare against. */
static void
gain_in_place (float *buf, size_t n, float g)
{
  for (size_t i = 0; i < n; i++)
    buf[i] *= g;
}

static void
gain_in_place_walk (float *buf, size_t n, float g)
{
  for (float *p = buf; p < buf + n; p++)
    *p *= g;
}

int
main (void)
{
  float  a[] = { 0.5f, -1.0f, 2.0f, 0.3f };
  float  b[] = { 0.5f, -1.0f, 2.0f, 0.3f };
  size_t n   = sizeof a / sizeof a[0];

  gain_in_place (a, n, 2.0f);
  gain_in_place_walk (b, n, 2.0f);
  printf ("doubled:");
  for (size_t i = 0; i < n; i++)
    printf (" %.2f", b[i]);
  printf ("\nidentical to the index version: %s\n",
          memcmp (a, b, sizeof a) == 0 ? "yes" : "no");
  return 0;
}
