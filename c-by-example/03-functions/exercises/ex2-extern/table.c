/* table.c (exercise 2) -- reaching into sine.c's counter.
 *
 * With `static` gone, `extern int made;` lets this file read made
 * directly. It also lets this file WRITE it, and so can every other file
 * in the program: the count is no longer sine.c's to keep.
 */
#include <stdio.h>

#include "sine.h"

extern int made; /* defined in sine.c, now visible everywhere */

int
main (void)
{
  float t[8];
  sine_table (t, 8, 1.0);
  sine_table (t, 8, 2.0);
  printf ("tables made: %d\n", made);

  made = -5; /* a typo, a bug, a careless caller: nothing stops it */
  printf ("tables made: %d (according to sine_tables_made: %d)\n", made,
          sine_tables_made ());
  return 0;
}
