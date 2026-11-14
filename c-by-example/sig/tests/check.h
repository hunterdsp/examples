/* check.h -- the whole test framework: one macro and one function.
 *
 * CHECK(cond) records a check, and if cond is false prints where and
 * what, and carries on, so one run reports every failure, not just the
 * first. A test program ends with `return check_report (argv[0]);`,
 * which prints a summary and exits non-zero if anything failed: that
 * exit status is all CTest looks at.
 *
 * Not assert(): a release build defines NDEBUG, which deletes every
 * assert, and the tests along with them (lesson 31).
 */
#ifndef SIG_TESTS_CHECK_H
#define SIG_TESTS_CHECK_H

#include <stdio.h>
#include <stdlib.h>

static int check_count, check_failed;

#define CHECK(cond)                                                           \
  do                                                                          \
    {                                                                         \
      check_count++;                                                          \
      if (!(cond))                                                            \
        {                                                                     \
          check_failed++;                                                     \
          printf ("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                     \
    }                                                                         \
  while (0)

/* The test's name: its path, without the directories. */
static const char *
check_name (const char *path)
{
  const char *name = path;
  for (const char *p = path; *p != '\0'; p++)
    if (*p == '/' || *p == '\\')
      name = p + 1;
  return name;
}

static int
check_report (const char *path)
{
  printf ("%s: %d of %d checks passed\n", check_name (path),
          check_count - check_failed, check_count);
  return check_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif /* SIG_TESTS_CHECK_H */
