/* internal.h -- what the library's own .c files share, and nobody else.
 *
 * It lives in src/, not include/, so it is never installed: a program
 * using sig can't include it, and the library may change it at will.
 * Its functions still have external linkage, since several .c files call
 * them, so they are named sig__ (two underscores), meaning "internal",
 * and the shared library hides them (lesson 34).
 */
#ifndef SIG_INTERNAL_H
#define SIG_INTERNAL_H

#define SIG_PI 3.14159265358979323846

/* 1 if a signal sampled at rate_hz can carry freq_hz: 0 < freq_hz <
   rate_hz / 2. Written so that a NaN fails every test. */
int sig__freq_ok (double freq_hz, double rate_hz);

#endif /* SIG_INTERNAL_H */
