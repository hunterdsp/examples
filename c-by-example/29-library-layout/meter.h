/* meter.h -- a header that compiles only after someone else's include.
 *
 * It uses size_t, from <stddef.h>, without including it. Any program
 * that happens to include <stdio.h> or <stdlib.h> first gets size_t
 * from them, and never notices.
 */
#ifndef METER_H
#define METER_H
float meter_peak (const float *buf, size_t n);
#endif
