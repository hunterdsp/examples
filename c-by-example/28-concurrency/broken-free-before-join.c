/* broken-free-before-join.c -- freeing what a thread still uses.
 *
 * main hands a buffer to a thread, then frees it before pthread_join
 * says the thread is done. Whether the thread has finished by then
 * depends on timing, which is exactly why this bug can pass a thousand
 * tests. Here a mutex makes the bad timing certain: the thread can't
 * start reading until main unlocks, and main frees first. AddressSanitizer
 * then sees the thread read freed memory.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;

static void *
reader (void *arg)
{
  const float *buf = arg;
  pthread_mutex_lock (&gate); /* waits until main lets it through */
  float sum = buf[0] + buf[1];
  pthread_mutex_unlock (&gate);
  printf ("%.1f\n", (double)sum);
  return NULL;
}

int
main (void)
{
  float *buf = calloc (2, sizeof *buf);
  if (buf == NULL)
    return 1;
  pthread_t t;
  pthread_mutex_lock (&gate);
  if (pthread_create (&t, NULL, reader, buf) != 0)
    return 1;
  free (buf);                   /* too early: the thread hasn't read it */
  pthread_mutex_unlock (&gate); /* ...and now it does */
  pthread_join (t, NULL);       /* the free belonged after this */
  return 0;
}
