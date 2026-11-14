/* test_common.c -- status descriptions and the version. */
#include "check.h"

#include <sig/common.h>

#include <string.h>

int
main (int argc, char *argv[])
{
  (void)argc;

  CHECK (strcmp (sig_strerror (SIG_OK), "success") == 0);
  CHECK (strcmp (sig_strerror (SIG_EINVAL), "argument out of range") == 0);
  CHECK (strcmp (sig_strerror (SIG_ENOMEM), "out of memory") == 0);
  CHECK (strcmp (sig_strerror ((sig_status)42), "unknown status") == 0);

  /* The library linked is the one these headers describe. */
  char want[32];
  snprintf (want, sizeof want, "%d.%d.%d", SIG_VERSION_MAJOR,
            SIG_VERSION_MINOR, SIG_VERSION_PATCH);
  CHECK (strcmp (sig_version (), want) == 0);

  return check_report (argv[0]);
}
