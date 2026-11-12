# 08 · The preprocessor and multi-file builds

```bash
make && ./meter
```

```
loud : rms 1.000, peak 1.000 =   0.00 dBFS
quiet: rms 0.500, peak 0.500 =  -6.02 dBFS
BAD_SQUARE(1 + 2) = 5, DSP_SQUARE(1 + 2) = 9
```

A level meter in three files: a header that promises (`dsp.h`), the code
that keeps the promise (`dsp.c`), and a program that uses it (`meter.c`).
Half amplitude reads −6 dB, as lesson 02 said it would.

## Translation units

The compiler never sees your *project*. It sees one `.c` file at a time,
after the preprocessor has pasted in its headers: a **translation unit**.
`meter.c` and `dsp.c` are compiled separately into `meter.o` and `dsp.o`,
and only the linker sees both. Two consequences:

- **Headers are how translation units agree.** Each `.c` that uses `dsp_rms`
  includes `dsp.h`, so each compiles its calls against the same declaration.
- **Change one file, recompile one file.** In a library of hundreds of
  files, that is the difference between a one-second and a ten-minute build.

## `make`

The [`Makefile`](Makefile) says what each file is built *from*:

```make
meter: meter.o dsp.o          # the program needs both objects
meter.o: meter.c dsp.h        # each object needs its source and the header
dsp.o: dsp.c dsp.h
```

`make` compares timestamps and runs only the recipes whose inputs changed.
Edit `dsp.c` and only `dsp.o` and `meter` rebuild; touch `dsp.h` and both
objects do. Recipe lines must start with a **tab**, not spaces.

## The preprocessor

- **Include guards.** `#ifndef DSP_H / #define DSP_H / … / #endif` makes a
  header's body appear once per translation unit, however often it's
  included. `meter.c` includes `dsp.h` twice to prove it. In a real project,
  headers include other headers, and without guards the same declaration
  would arrive twice.
- **Macros are text substitution**, done before compiling. That makes them
  fast and dangerous. `BAD_SQUARE(1 + 2)` expands to `1 + 2 * 1 + 2`, which
  is 5. `DSP_SQUARE` wraps every use of the argument, and the whole result,
  in parentheses: `((1 + 2) * (1 + 2))`, which is 9.

## Pitfalls

- **Linker errors.** [`broken-link.c`](broken-link.c) declares `dsp_rms` and
  calls it, but nothing in the build defines it. The compiler is satisfied
  (the declaration told it the shape); the linker is not:

  ```
  undefined reference to `dsp_rms'                     (Linux, GNU ld)
  Undefined symbols for architecture arm64: "_dsp_rms"  (macOS)
  ```

  Read it as "a `.c` file or a library is missing from the link line". Here,
  `dsp.c`.
- **Macro arguments evaluated twice.** `DSP_SQUARE(i++)` expands to
  `((i++) * (i++))`, which increments `i` twice and is undefined behaviour.
  Prefer a `static inline` function when you can: it has none of these
  problems, and costs nothing (Part III uses them).
- **Defining things in headers.** A header *declares*. A function or
  variable *defined* in a header is defined again in every `.c` that
  includes it, and the linker reports a duplicate. (`static inline`
  functions are the exception.)
- **Spaces instead of a tab in a Makefile**: `missing separator. Stop.`

## Exercises

1. Run `make`, then `make` again. Then `touch dsp.c` and `make`. Which
   commands ran each time?
2. Add `float dsp_crest_db(const float *buf, size_t n)` (peak over rms, in
   dB) to `dsp.h` and `dsp.c`. A sine wave's crest factor is 3.01 dB; what
   is a square wave's?
3. Rewrite `DSP_SQUARE` as a `static inline float dsp_square(float x)` in
   `dsp.h`, and check the output is unchanged.
