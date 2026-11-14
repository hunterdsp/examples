/* sig.h -- the public API of a tiny signal library.
 *
 * Everything here is a decision a caller will live with, so each one is
 * written down:
 *
 *   - Every public name starts with sig_ (or SIG_): C has one namespace.
 *   - Objects are opaque, made by _create and freed by _destroy.
 *   - Anything that can fail returns a sig_status; results come back
 *     through an out-parameter. sig_strerror says what went wrong.
 *   - Parameters are in the caller's units (Hz), not the library's.
 *   - Inputs are const; nothing in _process allocates, prints or blocks.
 *   - No global state: separate objects may be used from separate threads.
 */
#ifndef SIG_H
#define SIG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{ /* C++ callers: these are C functions, with C names */
#endif

/* The version this header describes. sig_version() reports the version
   of the library actually linked, which may differ. */
#define SIG_VERSION_MAJOR 1
#define SIG_VERSION_MINOR 0
#define SIG_VERSION_PATCH 0

  typedef enum
  {
    SIG_OK     = 0,
    SIG_EINVAL = -1, /* an argument is out of range */
    SIG_ENOMEM = -2  /* out of memory */
  } sig_status;

  /* A short, constant description of s: never NULL, never to be freed. */
  const char *sig_strerror (sig_status s);

  /* The linked library's version, as "MAJOR.MINOR.PATCH". */
  const char *sig_version (void);

  /* A one-pole low-pass filter (lesson 14), set by its cutoff frequency. */
  typedef struct sig_lowpass sig_lowpass;

  /* Make a filter for a signal sampled at rate_hz, passing frequencies
     below cutoff_hz. Needs 0 < cutoff_hz < rate_hz / 2. On SIG_OK, *out is
     the new filter, owned by the caller; on any error *out is NULL. */
  sig_status sig_lowpass_create (double cutoff_hz, double rate_hz,
                                 sig_lowpass **out);

  /* Free f. Accepts NULL. */
  void sig_lowpass_destroy (sig_lowpass *f);

  /* Filter n samples from in into out. in and out may be the same buffer
     (in place), but must not otherwise overlap. Never fails. */
  void sig_lowpass_process (sig_lowpass *f, const float *in, float *out,
                            size_t n);

  /* Forget the past, as if just created. */
  void sig_lowpass_reset (sig_lowpass *f);

#ifdef __cplusplus
}
#endif

#endif /* SIG_H */
