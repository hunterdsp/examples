# 22 · Oscillators

Run this lesson's commands in `c-by-example/22-oscillator/`.

An **oscillator** makes a periodic signal: a test tone, a synthesizer's voice,
the carrier a radio mixes with. Lesson 19 made a sine from `sin()` and from a
table. A real oscillator has one more job: to keep its pitch exactly, for as
long as it runs, which is hours.

Everything rests on the **phase**: how far through the current cycle the
oscillator is. Each sample, the phase moves on by an **increment**,
`frequency / sample rate` of a cycle, and at the end of each cycle it wraps
back to the start. How the phase is stored decides how well the oscillator
keeps time. This one stores it in a `uint32_t`, so that one whole cycle is the
whole range of the integer:

```bash
cat osc.c
```

```c
/* osc.c -- a sine oscillator with a 32-bit phase accumulator.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       osc.c -o osc -lm && ./osc
 *
 * The oscillator's PHASE says how far through the current cycle it is.
 * Here a whole cycle is the whole range of a uint32_t: 0 is the start,
 * 2^32 would be the start of the next, and since unsigned arithmetic
 * wraps at 2^32 (C defines it to), the phase wraps exactly where a cycle
 * ends, by itself, every time. Each sample adds the same INCREMENT;
 * the top 10 bits pick an entry of a 1024-entry sine table, and the low
 * 22 bits say how far it is to the next one.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>

enum
{
  TABLE_BITS = 10,
  TABLE      = 1 << TABLE_BITS, /* 1024 */
  FRAC_BITS  = 32 - TABLE_BITS  /* 22 */
};

static const double TWO_PI = 6.283185307179586;
static const double RATE   = 48000.0;
static const double CYCLE  = 4294967296.0; /* 2^32: one whole cycle */

static float table[TABLE + 1]; /* [TABLE] repeats [0], for k + 1 */

typedef struct
{
  uint32_t phase; /* where in the cycle, 0 .. 2^32 - 1 */
  uint32_t inc;   /* how far one sample moves it */
} osc_t;

static osc_t
osc_make (double freq_hz)
{
  osc_t o = { 0, (uint32_t)(freq_hz / RATE * CYCLE + 0.5) };
  return o;
}

static float
osc_next (osc_t *o)
{
  uint32_t k    = o->phase >> FRAC_BITS; /* top 10 bits: the entry */
  float    frac = (float)(o->phase & ((UINT32_C (1) << FRAC_BITS) - 1u))
                  / (float)(UINT32_C (1) << FRAC_BITS);
  o->phase += o->inc; /* wraps at 2^32: no if, no fmod */
  return table[k] + frac * (table[k + 1] - table[k]);
}

int
main (void)
{
  for (size_t k = 0; k <= TABLE; k++)
    table[k] = (float)sin (TWO_PI * (double)k / (double)TABLE);

  osc_t o = osc_make (440.0);
  printf ("440 Hz: increment %u of 2^32 per sample\n", (unsigned)o.inc);
  printf ("frequency played: %.4f Hz (steps of %.2e Hz)\n",
          (double)o.inc * RATE / CYCLE, RATE / CYCLE);

  /* Compare with sin() of a phase kept in double, over one second. */
  double worst = 0.0;
  for (size_t i = 0; i < (size_t)RATE; i++)
    {
      double t = (double)o.inc / CYCLE * (double)i; /* cycles so far */
      double d = fabs ((double)osc_next (&o) - sin (TWO_PI * t));
      worst    = d > worst ? d : worst;
    }
  printf ("one second: largest difference from sin() below 1e-5: %s\n",
          worst < 1e-5 ? "yes" : "no");
  printf ("after one second, the phase is at %.6f of a cycle\n",
          (double)o.phase / CYCLE);

  /* The tempting alternative: time in seconds, as a float. */
  const float when[] = { 1.0f, 60.0f, 600.0f, 3600.0f };
  for (size_t i = 0; i < sizeof when / sizeof when[0]; i++)
    {
      float t    = when[i];
      float next = t + (float)(1.0 / RATE); /* one sample later */
      printf ("float time at %5.0f s: next sample %s\n", (double)t,
              next == t ? "is the SAME time" : "is later");
    }
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined osc.c -o osc -lm
./osc
```

```
440 Hz: increment 39370534 of 2^32 per sample
frequency played: 440.0000 Hz (steps of 1.12e-05 Hz)
one second: largest difference from sin() below 1e-5: yes
after one second, the phase is at 0.000005 of a cycle
float time at     1 s: next sample is later
float time at    60 s: next sample is later
float time at   600 s: next sample is the SAME time
float time at  3600 s: next sample is the SAME time
```

## What it shows

- **Unsigned wraparound is the cycle.** Adding to a `uint32_t` past its largest
  value wraps around to 0, and C *defines* that it does: arithmetic on unsigned
  types is modulo 2³². With a cycle mapped onto that range, the phase wraps
  exactly at each cycle's end, with no `if` and no `fmod`, for as long as the
  oscillator runs.
- **The bits do the table lookup.** The top 10 bits of the phase pick one of
  1024 entries; the low 22 bits are the fraction of the way to the next one. A
  shift and a mask, where a floating-point phase would need a multiply and a
  conversion.
- **Exact, and repeatable.** 440 Hz is an increment of 39,370,534 per sample.
  That is not quite 440 Hz, but 440.000005, within a hundred-thousandth of a
  hertz; and integer addition never rounds, so the oscillator plays exactly
  that frequency, sample after sample, forever. One second in, it has drifted
  0.000005 of a cycle from where an ideal 440 Hz would be.
- **Time as a `float` runs out.** `sin(2πft)` with `t` in seconds looks
  simpler, but a `float` holds about seven significant digits. At 600 seconds,
  the gap between neighbouring `float`s is larger than one sample period, so
  `t + 1/48000` rounds back to `t`: time stops, and so does the wave. Long
  before that, every step is rounded, and the pitch is wrong.

## Pitfalls

- **A signed phase.** [`broken-signed-phase.c`](broken-signed-phase.c) keeps
  the phase in an `int32_t`. Signed overflow is undefined behaviour, not a wrap
  (lesson 11), and UBSan stops it within 55 samples:

  ```
  broken-signed-phase.c:18:11: runtime error: signed integer overflow: 2126008836 + 39370534 cannot be represented in type 'int'
  ```

  It might wrap anyway on your machine, until a compiler optimises on the
  assumption that it can't. Unsigned types only.

- **A phase that only grows.** Counting cycles instead of wrapping them loses
  precision as the count grows, and the pitch drifts (exercise 2).

- **Naive waveforms alias.** A saw or square wave computed straight from the
  phase (exercise 1) has sharp corners, and sharp corners contain frequencies
  above half the sample rate, which fold back down as unrelated, inharmonic
  tones. For a test signal that is fine; for a synthesizer, these waves need
  band-limiting, a subject of its own.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. The phase climbs steadily and wraps: plotted, it is already a waveform. Make
   a sawtooth, a square and a triangle wave from it, at 6 kHz, where one cycle
   is exactly 8 samples.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-waveforms/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined shapes.c -o shapes
   ./shapes
   ```

   ```
   phase     saw  square  triangle
   0.000   -1.00    1.00     -1.00
   0.125   -0.75    1.00     -0.50
   0.250   -0.50    1.00      0.00
   0.375   -0.25    1.00      0.50
   0.500    0.00   -1.00      1.00
   0.625    0.25   -1.00      0.50
   0.750    0.50   -1.00      0.00
   0.875    0.75   -1.00     -0.50
   ```

   The saw is the phase, stretched from 0…1 to −1…1. The square is +1 for the
   first half of each cycle and −1 for the second. The triangle folds the saw
   at zero, so it climbs for half a cycle and falls for the other half. Each is
   one line of [`shapes.c`](exercises/ex1-waveforms/shapes.c); all of them
   alias, as the pitfalls say.

   </details>

2. Keep the phase in a `float` instead, as a fraction of a cycle: once wrapping
   at 1.0, and once never wrapped, simply growing. Run each for a minute at 440
   Hz. What frequency does each actually play?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-float-phase/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined drift.c -o drift
   ./drift
   ```

   ```
   asked for 440.0000 Hz, over 60 s:
   float, wrapped:       439.9997 Hz
   float, never wrapped: 445.9207 Hz
   uint32_t:             440.0000 Hz
   ```

   The wrapped `float` is close: its phase stays below 1, where a `float` is
   fine-grained enough. The one that never wraps reaches 26,400 cycles by the
   end of the minute, where neighbouring `float`s are 0.002 cycles apart, and
   each step of 0.0092 cycles is rounded to a whole number of those. It plays
   445.9 Hz, nearly a quarter of a semitone sharp, an error anyone can hear,
   and it gets worse the longer it runs. The `uint32_t` phase never rounds.

   </details>

3. With an 8-, 12-, 16-, 24- or 32-bit phase at 48 kHz, what is the nearest
   frequency to 440 Hz each can play, and how far off is it, in cents?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-resolution/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined resolution.c -o resolution -lm
   ./resolution
   ```

   ```
   bits  step (Hz)    plays (Hz)  off by (cents)
      8  187.500000    375.000000      -276.7355
     12   11.718750    445.312500       +20.7775
     16    0.732422    440.185547        +0.7299
     24    0.002861    439.999580        -0.0017
     32    0.000011    440.000005        +0.0000
   ```

   An N-bit phase can only move by whole steps of `48000 / 2^N` Hz. With 16
   bits, the step is 0.73 Hz and the note is 0.73 cents sharp, about the limit
   a trained ear can hear; with 32 bits it is 11 *micro*hertz. That is why 32
   bits is the usual choice: an `int`-sized number, and as precise as anyone
   needs.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                    | header       | what it does                                                        |
| ----------------------------------------------------------- | ------------ | ------------------------------------------------------------------- |
| [`UINT32_C`](https://en.cppreference.com/c/types/integer)   | `<stdint.h>` | a constant of at least `uint32_t`'s range: `UINT32_C (1)` (a macro) |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)   | `<math.h>`   | absolute value of a `double`                                        |
| [`floor`](https://en.cppreference.com/c/numeric/math/floor) | `<math.h>`   | largest whole number not above x                                    |
| [`log2`](https://en.cppreference.com/c/numeric/math/log2)   | `<math.h>`   | base-2 logarithm                                                    |
| [`pow`](https://en.cppreference.com/c/numeric/math/pow)     | `<math.h>`   | x raised to the power y                                             |
| [`printf`](https://en.cppreference.com/c/io/fprintf)        | `<stdio.h>`  | formatted output to `stdout`                                        |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)     | `<math.h>`   | sine of an angle in radians                                         |

<!-- /libfuncs -->
