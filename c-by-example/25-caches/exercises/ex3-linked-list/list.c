/* list.c -- the same numbers in an array and in a linked list.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       list.c -o list && ./list
 *   cc -std=c99 -Wall -Wextra -O2 list.c -o list && ./list time
 *
 * Walking an array, the next element's address is known in advance, and
 * the processor fetches ahead. Walking a linked list, the next address
 * is INSIDE the current node, so nothing can be fetched until the
 * current fetch arrives. Here the nodes are linked in a shuffled order,
 * as they end up when a list is built and edited over time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
  N = 1 << 21 /* 2 million */
};

typedef struct node
{
  struct node *next;
  long         value;
} node;

int
main (int argc, char **argv)
{
  long   *array = malloc ((size_t)N * sizeof *array);
  node   *pool  = malloc ((size_t)N * sizeof *pool);
  size_t *order = malloc ((size_t)N * sizeof *order);
  if (array == NULL || pool == NULL || order == NULL)
    return 1;

  /* A shuffled visiting order (Fisher-Yates, with a fixed xorshift
     generator, so every run and every machine shuffles alike). */
  for (size_t i = 0; i < N; i++)
    order[i] = i;
  unsigned long long s = 88172645463325252ULL;
  for (size_t i = N - 1; i > 0; i--)
    {
      s ^= s << 13;
      s ^= s >> 7;
      s ^= s << 17;
      size_t j = (size_t)(s % (unsigned long long)(i + 1));
      size_t t = order[i];
      order[i] = order[j];
      order[j] = t;
    }
  for (size_t i = 0; i < N; i++)
    {
      array[i] = (long)i;
      node *n  = &pool[order[i]];
      n->value = (long)i;
      n->next  = i + 1 < N ? &pool[order[i + 1]] : NULL;
    }

  clock_t start = clock ();
  long    a_sum = 0;
  for (size_t i = 0; i < N; i++)
    a_sum += array[i];
  double t_array = (double)(clock () - start) / (double)CLOCKS_PER_SEC;

  start      = clock ();
  long l_sum = 0;
  for (const node *n = &pool[order[0]]; n != NULL; n = n->next)
    l_sum += n->value;
  double t_list = (double)(clock () - start) / (double)CLOCKS_PER_SEC;

  printf ("%d numbers: array sum %ld, list sum %ld\n", N, a_sum, l_sum);
  if (argc > 1 && strcmp (argv[1], "time") == 0)
    printf ("array: %6.1f ms\nlist:  %6.1f ms\n", t_array * 1e3, t_list * 1e3);
  free (array);
  free (pool);
  free (order);
  return 0;
}
