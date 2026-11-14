/* sharing.c -- four private counters, side by side or a line apart.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread \
 *       sharing.c -o sharing && ./sharing
 *   cc -std=c99 -Wall -Wextra -O2 -pthread sharing.c -o sharing \
 *       && ./sharing time
 *
 * Each thread counts into its own counter: no race, no lock needed. But
 * caches work in 64-byte lines (lesson 25), and a core must own a line
 * to write to it. Four counters packed into one line make the four cores
 * pass that line back and forth on every write: FALSE SHARING. Padding
 * each counter to a line of its own ends it.
 */
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
  THREADS = 4,
  EACH    = 50000000
};

static volatile long packed[THREADS]; /* 8 bytes apart: one line */

typedef struct
{
  volatile long n;
  char          pad[64 - sizeof (long)]; /* the rest of a 64-byte line */
} padded_counter;

static padded_counter padded[THREADS]; /* 64 bytes apart */

static void *
count (void *arg)
{
  volatile long *c = arg;
  for (long i = 0; i < EACH; i++)
    (*c)++;
  return NULL;
}

static double
now (void)
{
  struct timespec t;
  clock_gettime (CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

/* Run THREADS threads, thread k counting into counter[k]. */
static double
run (volatile long *const counter[THREADS])
{
  pthread_t t[THREADS];
  double    start = now ();
  for (int k = 0; k < THREADS; k++)
    pthread_create (&t[k], NULL, count, (void *)counter[k]);
  for (int k = 0; k < THREADS; k++)
    pthread_join (t[k], NULL);
  return now () - start;
}

int
main (int argc, char **argv)
{
  volatile long *a[THREADS], *b[THREADS];
  for (int k = 0; k < THREADS; k++)
    {
      a[k] = &packed[k];
      b[k] = &padded[k].n;
    }
  double t_packed = run (a);
  double t_padded = run (b);
  long   sa = 0, sb = 0;
  for (int k = 0; k < THREADS; k++)
    {
      sa += packed[k];
      sb += padded[k].n;
    }
  printf ("packed total %ld, padded total %ld\n", sa, sb);
  if (argc > 1 && strcmp (argv[1], "time") == 0)
    printf ("packed: %6.1f ms\npadded: %6.1f ms\n", t_packed * 1e3,
            t_padded * 1e3);
  return 0;
}
