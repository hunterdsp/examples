/* sine.h -- the PROMISE: what sine.c offers, and nothing about how.
 *
 * Any file that calls these includes this header, so the compiler can
 * check every call against the declaration. The include guard (#ifndef)
 * makes including it twice harmless; lesson 08 says more.
 */
#ifndef SINE_H
#define SINE_H

#include <stddef.h> /* size_t */

/* Fill out[0..n-1] with `cycles` periods of a unit sine wave. */
void sine_table (float out[], size_t n, double cycles);

/* How many tables sine_table has filled so far. */
int sine_tables_made (void);

#endif /* SINE_H */
