/**
 * @file common.h
 * @brief What every part of sig shares: status codes and the version.
 *
 * The rules of lesson 18 hold for the whole library:
 *
 *   - Every public name starts with sig_ (or SIG_): C has one namespace.
 *   - Objects are opaque, made by _create and freed by _destroy.
 *   - Anything that can fail returns a sig_status; results come back
 *     through an out-parameter. sig_strerror() says what went wrong.
 *   - Parameters are in the caller's units (Hz), not the library's.
 *   - Nothing in a _process function allocates, prints or blocks.
 *   - No global state: separate objects may be used from separate threads.
 */
#ifndef SIG_COMMON_H
#define SIG_COMMON_H

/** @cond INTERNAL */
/* SIG_API marks a function as part of the library's interface. A shared
   library is built with every other function hidden (lesson 34), so
   only these can be called from outside it. Windows DLLs mark them
   differently, with __declspec, which this course doesn't cover. */
#if defined __GNUC__
#define SIG_API __attribute__ ((visibility ("default")))
#else
#define SIG_API
#endif
/** @endcond */

#ifdef __cplusplus
extern "C"
{
#endif

/** The major version of the library this header describes. The build
    (lesson 30) reads all three numbers from here, so they are written
    once. */
#define SIG_VERSION_MAJOR 1
/** The minor version: new features, nothing removed. */
#define SIG_VERSION_MINOR 0
/** The patch version: fixes only. */
#define SIG_VERSION_PATCH 0

  /** What a function that can fail returns. */
  typedef enum
  {
    SIG_OK     = 0,  /**< Success. */
    SIG_EINVAL = -1, /**< An argument is out of range. */
    SIG_ENOMEM = -2  /**< Out of memory. */
  } sig_status;

  /**
   * Describe a status.
   *
   * @param s A status returned by a sig function.
   * @return A short, constant description: never NULL, never to be freed.
   */
  SIG_API const char *sig_strerror (sig_status s);

  /**
   * The version of the library actually linked, which may differ from
   * the SIG_VERSION_ macros the caller was compiled with.
   *
   * @return The version as "MAJOR.MINOR.PATCH".
   */
  SIG_API const char *sig_version (void);

#ifdef __cplusplus
}
#endif

#endif /* SIG_COMMON_H */
