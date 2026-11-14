/* queue.c -- one thread produces blocks, another consumes them.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread \
 *       queue.c -o queue && ./queue
 *
 * Lesson 12's ring buffer, shared between two threads. A mutex guards
 * it, and two CONDITION VARIABLES let a thread sleep until something
 * changes: the consumer while the ring is empty, the producer while it
 * is full. This is how an audio program hands blocks from a thread that
 * reads a file to the thread that plays them.
 */
#include <pthread.h>
#include <stdio.h>

enum
{
  CAP   = 8, /* a small ring, so the producer often has to wait */
  ITEMS = 10000
};

typedef struct
{
  long            item[CAP];
  size_t          head, tail; /* free-running, as in lesson 12 */
  pthread_mutex_t lock;
  pthread_cond_t  not_empty, not_full;
} queue;

static queue q = { .lock      = PTHREAD_MUTEX_INITIALIZER,
                   .not_empty = PTHREAD_COND_INITIALIZER,
                   .not_full  = PTHREAD_COND_INITIALIZER };

static void
put (queue *qu, long v)
{
  pthread_mutex_lock (&qu->lock);
  while (qu->head - qu->tail == CAP) /* full: sleep until a get */
    pthread_cond_wait (&qu->not_full, &qu->lock);
  qu->item[qu->head % CAP] = v;
  qu->head++;
  pthread_cond_signal (&qu->not_empty); /* wake a sleeping get */
  pthread_mutex_unlock (&qu->lock);
}

static long
get (queue *qu)
{
  pthread_mutex_lock (&qu->lock);
  while (qu->head == qu->tail) /* empty: sleep until a put */
    pthread_cond_wait (&qu->not_empty, &qu->lock);
  long v = qu->item[qu->tail % CAP];
  qu->tail++;
  pthread_cond_signal (&qu->not_full);
  pthread_mutex_unlock (&qu->lock);
  return v;
}

static void *
producer (void *arg)
{
  (void)arg;
  for (long v = 1; v <= ITEMS; v++)
    put (&q, v);
  return NULL;
}

int
main (void)
{
  pthread_t t;
  if (pthread_create (&t, NULL, producer, NULL) != 0)
    return 1;
  long sum = 0, last = 0, in_order = 1;
  for (int i = 0; i < ITEMS; i++) /* main is the consumer */
    {
      long v   = get (&q);
      in_order = in_order && v == last + 1;
      last     = v;
      sum += v;
    }
  pthread_join (t, NULL);
  printf ("received %d items, in order: %s, sum %ld\n", ITEMS,
          in_order ? "yes" : "no", sum);
  return 0;
}
