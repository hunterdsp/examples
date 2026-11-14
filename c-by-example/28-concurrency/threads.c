/* threads.c -- four channels, filtered on four threads.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread \
 *       threads.c -o threads -lm && ./threads
 *   cc -std=c99 -Wall -Wextra -O2 -pthread threads.c -o threads -lm \
 *       && ./threads time
 *
 * A THREAD is a second (third, ...) path of execution through the same
 * program, sharing all its memory, which the operating system can run on
 * another processor core at the same time. Threads come from POSIX, not
 * from C99: <pthread.h>, and -pthread when building.
 *
 * The easy case, and the best one, is work that splits into pieces that
 * share nothing: here, the channels of a recording, each filtered on its
 * own, each by its own thread. No locks are needed, because no thread
 * ever touches another's data.
 */
#define _POSIX_C_SOURCE 200809L /* clock_gettime: POSIX, beyond C99 */

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
  CHANNELS = 4,
  N        = 1 << 22 /* samples per channel: 87 s at 48 kHz */
};

/* One channel's job: everything a thread needs, in one struct. */
typedef struct
{
  float *x; /* the samples, filtered in place */
  size_t n;
  float  a;     /* one-pole low-pass coefficient (lesson 14) */
  double total; /* the result: the sum of the output */
} job;

/* A thread's starting function: takes and returns a void * (lesson 15).
   This one is handed its job. */
static void *
filter_channel (void *arg)
{
  job  *j = arg;
  float y = 0.0f;
  for (size_t i = 0; i < j->n; i++)
    {
      y += j->a * (j->x[i] - y);
      j->x[i] = y;
    }
  double total = 0.0;
  for (size_t i = 0; i < j->n; i++)
    total += (double)j->x[i];
  j->total = total; /* written only by this thread, read after the join */
  return NULL;
}

static void
fill (float *x, size_t n, size_t channel)
{
  for (size_t i = 0; i < n; i++)
    x[i] = (float)sin (0.001 * (double)(channel + 1) * (double)i);
}

/* Wall-clock seconds. clock() would add up the CPU time of every thread,
   and so hide exactly the speed-up being measured. */
static double
now (void)
{
  struct timespec t;
  clock_gettime (CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

int
main (int argc, char **argv)
{
  float *data = malloc ((size_t)CHANNELS * N * sizeof *data);
  if (data == NULL)
    return 1;
  job jobs[CHANNELS];
  for (size_t c = 0; c < CHANNELS; c++)
    jobs[c] = (job){ data + c * N, N, 0.01f, 0.0 };

  /* One thread after another, on this one. */
  for (size_t c = 0; c < CHANNELS; c++)
    fill (jobs[c].x, N, c);
  double start = now ();
  double one[CHANNELS];
  for (size_t c = 0; c < CHANNELS; c++)
    {
      filter_channel (&jobs[c]);
      one[c] = jobs[c].total;
    }
  double t_one = now () - start;

  /* All four at once, one thread each. */
  for (size_t c = 0; c < CHANNELS; c++)
    fill (jobs[c].x, N, c);
  start = now ();
  pthread_t tid[CHANNELS];
  for (size_t c = 0; c < CHANNELS; c++)
    if (pthread_create (&tid[c], NULL, filter_channel, &jobs[c]) != 0)
      return 1;
  for (size_t c = 0; c < CHANNELS; c++)
    pthread_join (tid[c], NULL); /* wait for each to finish */
  double t_four = now () - start;

  int same = 1;
  for (size_t c = 0; c < CHANNELS; c++)
    same = same && jobs[c].total == one[c];
  printf ("%d channels: threaded results identical: %s\n", CHANNELS,
          same ? "yes" : "no");
  if (argc > 1 && strcmp (argv[1], "time") == 0)
    printf ("one thread:   %5.1f ms\nfour threads: %5.1f ms\n", t_one * 1e3,
            t_four * 1e3);
  free (data);
  return 0;
}
