/* compat.c -- refuse a library this program wasn't built for.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       compat.c sig.c -o compat -lm && ./compat
 *
 * The header's version is fixed when the program is COMPILED; the
 * library's when it is LINKED, or, for a shared library, when the
 * program starts. A different MAJOR version means an incompatible API,
 * so the program checks before it calls anything else.
 */
#include "sig.h"

#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
  const char *running = sig_version ();
  long        major   = strtol (running, NULL, 10); /* "2.0.0" -> 2 */
  printf ("header %d.%d.%d, library %s: ", SIG_VERSION_MAJOR,
          SIG_VERSION_MINOR, SIG_VERSION_PATCH, running);
  if (major != (long)SIG_VERSION_MAJOR)
    {
      printf ("incompatible, stopping\n");
      return 1;
    }
  printf ("compatible\n");
  return 0;
}
