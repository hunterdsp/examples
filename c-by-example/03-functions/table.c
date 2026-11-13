/* table.c -- calling functions, and what "pass by value" means.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       table.c sine.c -o table -lm && ./table
 */
#include <stdio.h>

#include "sine.h"

/* Arguments are COPIES. `x` here is a new variable holding the caller's
   value; changing it changes nothing outside. (Lesson 05 shows how a
   function changes its caller's data: pass where the data lives.) */
static void
try_to_double (int x)
{
  x = x * 2;
  printf ("  inside:  x = %d\n", x);
}

/* An array argument is passed as the address of its first element, so
   the function CAN write into the caller's array; `const` promises this
   one won't. More in lesson 04. */
static void
print_table (const char *label, const float table[], size_t n)
{
  printf ("%s:", label);
  for (size_t i = 0; i < n; i++)
    printf (" %7.4f", table[i]);
  printf ("\n");
}

/* `static` INSIDE a function means something else: the variable is made
   once, when the program starts, and keeps its value between calls. A
   plain local is made afresh on every call. */
static void
count_calls (void)
{
  static int kept  = 0;
  int        fresh = 0;
  kept++;
  fresh++;
  printf ("  kept = %d, fresh = %d\n", kept, fresh);
}

int
main (void)
{
  int n = 21;
  try_to_double (n);
  printf ("  outside: n = %d\n", n);

  float one[8], two[8];
  sine_table (one, 8, 1.0); /* one period over 8 samples */
  sine_table (two, 8, 2.0); /* two periods: twice the frequency */
  print_table ("1 cycle ", one, 8);
  print_table ("2 cycles", two, 8);
  printf ("tables made: %d\n", sine_tables_made ());

  count_calls ();
  count_calls ();
  count_calls ();
  return 0;
}
