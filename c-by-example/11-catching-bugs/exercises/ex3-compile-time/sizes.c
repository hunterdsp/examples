/* sizes.c (exercise 3) -- an assertion checked by the compiler.
 *
 * assert() checks while the program runs, and only the paths that run.
 * Some facts are known while COMPILING, such as the size of a type, and
 * can be checked then, before the program exists. C11 has
 * _Static_assert for this; in C99 the trick is an array whose size is -1
 * when the condition is false, which no compiler will accept.
 */
#include <stdio.h>

/* Declares a type that is never used: only the check matters. Its size
   is 1 if cond is true, and -1, an error, if not. */
#define STATIC_CHECK(cond, name)                                              \
  typedef char static_check_##name[(cond) ? 1 : -1]

/* Lesson 06 writes floats to files as 4 bytes: make that a build error
   on any machine where it is not true, rather than a corrupt file. */
STATIC_CHECK (sizeof (float) == 4, float_is_4_bytes);
STATIC_CHECK (sizeof (double) == 8, double_is_8_bytes);

int
main (void)
{
  printf ("float is %zu bytes, double %zu: checked when compiling\n",
          sizeof (float), sizeof (double));
  return 0;
}
