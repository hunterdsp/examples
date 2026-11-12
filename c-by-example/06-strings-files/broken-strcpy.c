/* broken-strcpy.c -- a string that does not fit.
 *
 * strcpy copies until it finds the '\0', however far that is. Here the
 * name is longer than the 8 bytes it is copied into, so the copy runs off
 * the end of the array: the classic buffer overflow.
 */
#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  /* With no arguments, the name is this program's own path: far longer
     than 8 bytes. */
  const char *name = argc > 1 ? argv[1] : argv[0];
  char copy[8];
  strcpy (copy, name); /* should be snprintf (copy, sizeof copy, ...) */
  printf ("%s\n", copy);
  return 0;
}
