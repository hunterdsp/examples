/* broken-no-main.c (exercise 3) -- stages.c with main renamed to start.
 *
 * Preprocessing, compiling and assembling all succeed: start is a fine
 * name. The linker then finds the C library's startup code calling a
 * main that nothing defines, and stops: stage 4.
 */
#include <stdio.h>

#define GREETING "compiled, linked, ran"

int
start (void)
{
  puts (GREETING);
  return 0;
}
