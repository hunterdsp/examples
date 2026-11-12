/* hello.c (exercise 1) -- the same program, reporting failure.
 *
 * Returning 3 instead of 0 tells the shell the program failed, so `&&`
 * refuses to run whatever comes after it.
 */
#include <stdio.h>

int
main (int argc, char *argv[])
{
  printf ("Hello, World!\n");

  /* argv[0] is how the program was invoked; argc counts every word on the
     command line, including that one. */
  printf ("called with %d argument(s)\n", argc - 1);
  for (int i = 1; i < argc; i++)
    printf ("  argv[%d] = %s\n", i, argv[i]);

  /* 0 tells the shell "success". Anything else is a failure code, which
     is how `make`, `&&` and `if` know your program went wrong. */
  return 3; /* anything but 0 tells the shell "it failed" */
}
