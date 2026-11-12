/* hello.c (exercise 2) -- argv[0] is whatever you typed to run it.
 *
 * The shell passes the program's name exactly as it was given, path and
 * all. It is not the file's "real" name; a program that needs to find
 * files next to itself cannot rely on it.
 */
#include <stdio.h>

int
main (int argc, char *argv[])
{
  (void)argc;
  printf ("argv[0] = %s\n", argv[0]);
  return 0;
}
