/* counter.c -- four threads counting into one variable.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread \
 *       counter.c -o counter && ./counter
 *   ./counter race
 *
 * count++ looks like one step, but is three: read count, add 1, write it
 * back. Two threads doing that at once can both read 41 and both write
 * 42: one count is lost. A MUTEX (a lock) lets one thread in at a time.
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>

enum
{
  THREADS = 4,
  EACH    = 1000000
};

static long            count;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *
count_locked (void *arg)
{
  (void)arg;
  for (int i = 0; i < EACH; i++)
    {
      pthread_mutex_lock (&lock); /* one thread at a time from here... */
      count++;
      pthread_mutex_unlock (&lock); /* ...to here */
    }
  return NULL;
}

static void *
count_racing (void *arg)
{
  (void)arg;
  for (int i = 0; i < EACH; i++)
    count++; /* a DATA RACE: undefined behaviour */
  return NULL;
}

static long
run (void *(*fn) (void *))
{
  pthread_t t[THREADS];
  count = 0;
  for (int k = 0; k < THREADS; k++)
    pthread_create (&t[k], NULL, fn, NULL);
  for (int k = 0; k < THREADS; k++)
    pthread_join (t[k], NULL);
  return count;
}

int
main (int argc, char **argv)
{
  printf ("expected: %ld\n", (long)THREADS * EACH);
  printf ("locked:   %ld\n", run (count_locked));
  if (argc > 1 && strcmp (argv[1], "race") == 0)
    printf ("racing:   %ld\n", run (count_racing));
  return 0;
}
