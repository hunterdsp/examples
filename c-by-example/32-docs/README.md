# 32 · Documentation

Run this lesson's commands in `c-by-example/32-docs/`.

A library's user reads its documentation, not its source. Lesson 18 put the
promises in the header's comments, because the header is the one file every
user has. **Doxygen** reads those comments and makes a website of them: a page
per header, an entry per function, with its parameters, what it returns, and
links between them. It is the usual tool for C and C++ APIs, and the comments
it reads stay plain comments to the compiler and to anyone reading the header.

Doxygen reads only comments marked for it, which start `/**`. `sig`'s biquad
header:

```bash
cat ../sig/include/sig/biquad.h
```

```c
/**
 * @file biquad.h
 * @brief Low-pass and high-pass biquad filters.
 *
 * Lesson 21's second-order IIR filter, designed with the formulas of
 * Robert Bristow-Johnson's "Audio EQ Cookbook" and run in transposed
 * direct form II.
 */
#ifndef SIG_BIQUAD_H
#define SIG_BIQUAD_H

#include <sig/common.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** Which frequencies a biquad passes. */
  typedef enum
  {
    SIG_LOWPASS, /**< Below the corner frequency. */
    SIG_HIGHPASS /**< Above the corner frequency. */
  } sig_biquad_type;

  /** A biquad filter. Opaque: made by sig_biquad_create(). */
  typedef struct sig_biquad sig_biquad;

  /**
   * Make a filter.
   *
   * @param type SIG_LOWPASS or SIG_HIGHPASS.
   * @param f0_hz The corner frequency. Needs 0 < f0_hz < rate_hz / 2.
   * @param q The shape of the corner, > 0. 0.7071 gives the flattest
   *          passband (a Butterworth filter); higher values peak just
   *          before the corner.
   * @param rate_hz The sample rate, in samples per second.
   * @param[out] out On SIG_OK, the new filter, owned by the caller; on any
   *                 error, NULL.
   * @return SIG_OK, SIG_EINVAL or SIG_ENOMEM.
   */
  SIG_API sig_status sig_biquad_create (sig_biquad_type type, double f0_hz,
                                        double q, double rate_hz,
                                        sig_biquad **out);

  /**
   * Free a filter.
   *
   * @param f The filter, or NULL, which does nothing.
   */
  SIG_API void sig_biquad_destroy (sig_biquad *f);

  /**
   * Filter n samples. in and out may be the same buffer (in place), but
   * must not otherwise overlap.
   *
   * @param f The filter.
   * @param in The n samples to filter.
   * @param[out] out Where the n filtered samples go.
   * @param n How many samples.
   */
  SIG_API void sig_biquad_process (sig_biquad *f, const float *in, float *out,
                                   size_t n);

  /**
   * Forget the past, as if just created.
   *
   * @param f The filter.
   */
  SIG_API void sig_biquad_reset (sig_biquad *f);

#ifdef __cplusplus
}
#endif

#endif /* SIG_BIQUAD_H */
```

## Building it

Doxygen's settings are a text file, `docs/Doxyfile.in`, and CMake fills in the
version and the build folder (`configure_file`, in `CMakeLists.txt`) to make
`build/Doxyfile`:

```bash
cat ../sig/docs/Doxyfile.in
```

```
# Doxyfile.in -- Doxygen's settings for sig's API documentation. CMake
# fills in the @...@ values and writes the result into the build
# directory (configure_file). Only settings that differ from Doxygen's
# defaults are here; `doxygen -g` writes a file listing every one.

PROJECT_NAME           = sig
PROJECT_NUMBER         = @PROJECT_VERSION@
PROJECT_BRIEF          = "A small signal-processing library"
OUTPUT_DIRECTORY       = @CMAKE_CURRENT_BINARY_DIR@/docs

# The public headers are the API; the README is the front page.
INPUT                  = include/sig README.md
USE_MDFILE_AS_MAINPAGE = README.md
OPTIMIZE_OUTPUT_FOR_C  = YES

# Every public name must be documented, and every parameter of every
# function: anything missing is a warning, and a warning fails the build.
EXTRACT_ALL            = NO
WARN_IF_UNDOCUMENTED   = YES
WARN_NO_PARAMDOC       = YES
WARN_AS_ERROR          = FAIL_ON_WARNINGS
QUIET                  = YES

# HTML only: no LaTeX, and no diagrams, which need Graphviz.
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
HAVE_DOT               = NO
```

CMake makes a `docs` target only if it finds Doxygen (`find_package`), so
building `sig` never needs it, and building the documentation is one more
target:

```bash
cmake -S ../sig -B build
cmake --build build
cmake --build build --target docs
```

The website is `build/docs/html/`, and its front page is `index.html`, made
from `../sig/README.md`:

```bash
test -e build/docs/html/index.html && echo "the documentation is built"
```

```
the documentation is built
```

Open `build/docs/html/index.html` in a browser, and follow the *Files* tab to
`biquad.h`.

## What it shows

- **The tags.** `@brief` is the one-line summary. `@param` describes a
  parameter, and `@param[out]` one the function writes to. `@return` says what
  comes back. `/**<` documents the thing to its *left*, which is how an enum's
  members get a line each. `@file` documents the header itself, and Doxygen
  lists a file's functions only once the file is documented.
- **Missing documentation is an error.** `WARN_IF_UNDOCUMENTED` and
  `WARN_NO_PARAMDOC` warn about any public name or parameter without a
  description, and `WARN_AS_ERROR = FAIL_ON_WARNINGS` makes any warning fail
  the build (exercise 1). A new function can't arrive undocumented.
- **Only the interface.** `INPUT` is `include/sig` and the README: the
  documentation is the API, so `src/` isn't in it. `@cond INTERNAL` …
  `@endcond` in `common.h` hides `SIG_API`, which callers never use.
- **One version, again.** `PROJECT_NUMBER = @PROJECT_VERSION@` is filled in by
  CMake, from `common.h`'s three `#define`s (exercise 3).
- **The README is the front page.** `USE_MDFILE_AS_MAINPAGE` uses it, so the
  page GitHub shows and the documentation's first page are one file.

## Pitfalls

- **Comments that repeat the code.** "`@param n` the n" passes every check and
  helps no one. Say what the code can't: units (Hz, not radians), ranges, who
  owns what, what happens to `*out` on failure, whether two buffers may
  overlap.

- **Documentation that drifted.** Doxygen checks that a description exists, not
  that it is still true. An example that compiles and runs can't drift
  unnoticed, if something runs it: exercise 2 runs the README's example, the
  way this course runs every README.

- **Links that only work in one place.** The README is read on GitHub and by
  Doxygen, and a relative link to a file outside `INPUT` works on GitHub and
  fails the Doxygen build, which is why `sig`'s README links to the course by
  its full address.

## Exercises

Try each one before opening its answer. All three use the lesson's own folder,
and its build.

1. Delete the description of `sig_osc_process`'s `n` from a copy of `osc.h`,
   and run Doxygen on the copy.

   <details><summary>Output and answer</summary>

   `sed '/…/d'` deletes the matching line:

   ```bash
   mkdir -p build/sabotage
   sed '/@param n How many samples\./d' ../sig/include/sig/osc.h > build/sabotage/osc.h
   diff ../sig/include/sig/osc.h build/sabotage/osc.h
   ```

   ```
   49d48
   <    * @param n How many samples.
   ```

   `doxygen -` reads its settings from standard input, and a setting given
   twice takes the later value, so this is `sig`'s Doxyfile, pointed at the
   copy:

   ```bash
   if (cat build/Doxyfile
       echo "INPUT = build/sabotage"
       echo "USE_MDFILE_AS_MAINPAGE ="
       echo "OUTPUT_DIRECTORY = build/sabotage/docs") | doxygen - 2>/dev/null; then
     echo "documentation built"
   else
     echo "documentation build failed"
   fi
   ```

   ```
   documentation build failed
   ```

   Doxygen 1.15 says why (2> sends it to `/dev/null` above, since its wording
   differs between versions):

   ```
   …/build/sabotage/osc.h:44: error: The following parameter of sig_osc_process(sig_osc *o, float *out, size_t n) is not documented:
     parameter 'n'
   ```

   A missing description fails the build, the same way a failing test does, so
   it can be a check in CI. Every rule worth keeping needs one: a rule that
   nothing enforces is broken the first time someone is in a hurry.

   </details>

2. `../sig/README.md` shows a program using `sig`, and the tools in a pipe,
   with their output. Check that both still work as written.

   <details><summary>Output and answer</summary>

   The first `sed` prints the lines from a line that is exactly ```` ```c ````
   to the next ```` ``` ````, and the second deletes the first and last of
   those (`1d;$d`), leaving the program:

   ````bash
   sed -n '/^```c$/,/^```$/p' ../sig/README.md | sed '1d;$d' > build/use.c
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../sig/include build/use.c -L build -lsig -lm -o build/use
   build/use
   ````

   ```
   sig 1.0.0: 0.0000 0.0576 0.1149 0.1719
   ```

   The README runs its tools from `sig`'s folder, with a build in `build/`.
   This lesson's build is in a `build/` too, so its command works here,
   unchanged:

   ```bash
   build/sig-tone 1 750 6000 | build/sig-filter lowpass 1500 | build/sig-fft
   ```

   ```
     750.00 Hz  0.485
    6000.00 Hz  0.028
   ```

   Both match what the README says. This is how the course keeps its own
   READMEs true: `check.sh` runs every command in them and compares the output.
   Copying the example out with `sed`, instead of keeping a second copy, means
   the README itself is what gets tested.

   </details>

3. Where does the `1.0.0` at the top of the documentation's pages come from?

   <details><summary>Output and answer</summary>

   ```bash
   grep '^PROJECT_NUMBER' build/Doxyfile
   grep '#define SIG_VERSION_' ../sig/include/sig/common.h
   ```

   ```
   PROJECT_NUMBER         = 1.0.0
   #define SIG_VERSION_MAJOR 1
   #define SIG_VERSION_MINOR 0
   #define SIG_VERSION_PATCH 0
   ```

   From `common.h`. `CMakeLists.txt` reads the three numbers into
   `PROJECT_VERSION`, and `configure_file` writes it into `build/Doxyfile`. The
   same number names the shared library and goes into the installed package
   files (lesson 34), and `sig_version()` builds its string from the same
   `#define`s. One place to change it, and nothing that can disagree.

   </details>
