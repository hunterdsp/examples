# 34 · Distribution

Run this lesson's commands in `c-by-example/34-distribution/`.

So far every program that used `sig` was built next to its source. A library is
*distributed* when other people's programs use it without ever seeing its
source: its headers and compiled library are **installed** somewhere, and other
projects are told where. This lesson makes `sig` installable, as a shared
library, and uses it the two ways other projects will: with `pkg-config`, and
with CMake's `find_package`.

## Static and shared

Lesson 29's `libsig.a` is a **static library**: the linker copies the object
files a program needs into the program, which then needs nothing else. A
**shared library** is linked when the program *starts*: the program records
only the library's name, and the system's loader finds the library and connects
the two each time the program runs. One copy on disk and in memory serves every
program, and a fixed library fixes them all without relinking. The price is
that the library must be there, and must still match.

`BUILD_SHARED_LIBS=ON` makes `add_library` build a shared library, and
`Release` makes it an optimised build, as one meant for others should be:

```bash
cmake -S ../sig -B build -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On Linux it is `build/libsig.so.1.0.0`, with two links to it, `libsig.so.1` and
`libsig.so`. On macOS it is `build/libsig.1.0.0.dylib`, with `libsig.1.dylib`
and `libsig.dylib`. The three names are three promises (they come from
`VERSION` and `SOVERSION` in `CMakeLists.txt`):

| name              | used by                 | means                               |
| ----------------- | ----------------------- | ----------------------------------- |
| `libsig.so.1.0.0` | nobody, directly        | this exact build                    |
| `libsig.so.1`     | programs, when they run | any 1.x: the *soname*               |
| `libsig.so`       | the linker, for `-lsig` | whichever is installed for building |

A program linked with `-lsig` records the soname, `libsig.so.1`, so it runs
with any later 1.x, and refuses a 2.0, whose changes may break it.

## Installing

`cmake --install` copies what a user needs into a *prefix*: headers into
`include/`, libraries into `lib/`, tools into `bin/`. A system-wide install
uses `/usr/local`, which needs administrator rights; this one stays in the
lesson's folder:

```bash
cmake --install build --prefix staging
```

```bash
ls staging staging/include/sig staging/bin
```

```
staging:
bin
include
lib

staging/bin:
sig-fft
sig-filter
sig-tone

staging/include/sig:
biquad.h
common.h
fft.h
osc.h
sig.h
```

Only `include/sig/` was installed, not `src/`: `internal.h` stays private
(lesson 29). `staging/lib` holds the library, and two descriptions of it for
other projects to read: `pkgconfig/sig.pc` and `cmake/sig/`. Some Linux
systems, Fedora among them, use `lib64` instead of `lib`.

The installed tools work, from anywhere:

```bash
staging/bin/sig-tone 1 750 | staging/bin/sig-fft
```

```
  750.00 Hz  0.500
```

## Using it: pkg-config

**pkg-config** answers one question for a build: what flags does this library
need? It reads `.pc` files, and `PKG_CONFIG_PATH` adds folders for it to look
in. `sig`'s, made by CMake from `../sig/sig.pc.in`:

```bash
cat staging/lib/pkgconfig/sig.pc
```

```
# sig.pc -- how to compile and link against sig, for pkg-config.
# The prefix is worked out from where this file is (pcfiledir), not
# written in, so the installed tree can be moved and still work.
prefix=${pcfiledir}/../..
includedir=${prefix}/include
libdir=${prefix}/lib

Name: sig
Description: A small signal-processing library, from Learning C by Example
Version: 1.0.0
Cflags: -I${includedir}
Libs: -L${libdir} -lsig
Libs.private: -lm
```

```bash
PKG_CONFIG_PATH=staging/lib/pkgconfig pkg-config --modversion sig
PKG_CONFIG_PATH=staging/lib/pkgconfig pkg-config --cflags --libs sig
```

```
1.0.0
-Istaging/lib/pkgconfig/../../include -Lstaging/lib/pkgconfig/../../lib -lsig
```

A program built against the installed `sig`, not `../sig`:

```bash
cat analyse.c
```

```c
/* analyse.c -- a program built against an INSTALLED sig: the headers and
 * library under staging/, found with pkg-config, not ../sig.
 *
 * It does what `sig-tone 1 750 6000 | sig-filter lowpass 1500 | sig-fft`
 * does, in one program.
 */
#include <sig/sig.h>

#include <stdio.h>

enum
{
  RATE = 48000,
  N    = 4096
};

int
main (void)
{
  static float tone[RATE], x[RATE], amp[N / 2 + 1];
  sig_osc     *a, *b;
  sig_biquad  *lp;
  sig_fft     *fft;
  if (sig_osc_create (750.0, RATE, &a) != SIG_OK
      || sig_osc_create (6000.0, RATE, &b) != SIG_OK
      || sig_biquad_create (SIG_LOWPASS, 1500.0, 0.7071, RATE, &lp) != SIG_OK
      || sig_fft_create (N, &fft) != SIG_OK)
    return 1; /* a demo: the system frees what was made */

  sig_osc_process (a, x, RATE);
  sig_osc_process (b, tone, RATE);
  for (size_t i = 0; i < RATE; i++)
    x[i] = 0.5f * x[i] + 0.5f * tone[i];
  sig_biquad_process (lp, x, x, RATE);
  sig_fft_amplitude (fft, x + RATE - N, amp); /* the last N samples */

  printf ("sig %s, installed\n", sig_version ());
  for (size_t k = 0; k <= N / 2; k++)
    if (amp[k] > 0.01f)
      printf ("%8.2f Hz  %.3f\n", (double)k * (double)RATE / (double)N,
              (double)amp[k]);

  sig_fft_destroy (fft);
  sig_biquad_destroy (lp);
  sig_osc_destroy (b);
  sig_osc_destroy (a);
  return 0;
}
```

`$( … )` puts pkg-config's answer into the command line. `-Wl,-rpath,…` is
explained below:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined analyse.c $(PKG_CONFIG_PATH=staging/lib/pkgconfig pkg-config --cflags --libs sig) -Wl,-rpath,"$PWD/staging/lib" -o analyse
./analyse
```

```
sig 1.0.0, installed
  750.00 Hz  0.485
 6000.00 Hz  0.028
```

The same numbers as lesson 33's pipe, from one program.

## What it shows

- **Only `SIG_API` is exported.** `C_VISIBILITY_PRESET hidden` compiles the
  library with `-fvisibility=hidden`, so a shared library's functions are
  invisible from outside it unless marked. `SIG_API` marks the public ones
  `visibility("default")`. `sig__freq_ok` is hidden: no program can come to
  depend on it, and the loader has fewer names to look up (exercise 3).
- **The prefix isn't written in.** `prefix=${pcfiledir}/../..` makes the `.pc`
  file find the rest of the install relative to itself, and `INSTALL_RPATH`
  (`$ORIGIN/../lib`, `@loader_path/../lib` on macOS) does the same for the
  installed tools. The tree can be moved and keeps working (exercise 2).
- **The sanitizers stay home.** `SIG_SANITIZE` adds its flags inside
  `$<BUILD_INTERFACE:…>`, so they apply in `sig`'s own build tree and are left
  out of the installed `sigTargets.cmake`. A user of the installed library
  doesn't inherit them.
- **Versions are promises.** Semantic versioning: a fix is 1.0.1; a new
  function, which breaks nothing, 1.1.0; removing or changing anything a
  program can depend on, 2.0.0, with a new soname, `libsig.so.2`. Opaque types
  (lesson 14) help: `sig_osc` can change size without breaking anyone, because
  no program knows its size. A public struct can't.

## Pitfalls

- **The loader can't find it.** The linker found `libsig.so` because `-L` said
  where. The loader, when the program runs, doesn't read `-L`, and looks only
  in the system's folders and the folders recorded in the program as its
  *rpath*. Without `-Wl,-rpath,…` (`-Wl,` passes the rest to the linker), Linux
  says:

  <!-- no-run -->

  ```bash
  cc -std=c99 analyse.c $(PKG_CONFIG_PATH=staging/lib/pkgconfig pkg-config --cflags --libs sig) -o analyse
  ./analyse
  ```

  ```
  ./analyse: error while loading shared libraries: libsig.so.1: cannot open shared object file: No such file or directory
  ```

  An install into a system folder, like `/usr/local/lib`, needs no rpath.
  `LD_LIBRARY_PATH` (`DYLD_LIBRARY_PATH` on macOS) also tells the loader where
  to look, for one command, and is fine for trying something, but a program
  that only runs with it set is broken.

- **A prefix written in at configure time.** A `.pc` file with
  `prefix=/usr/local` in it, when `cmake --install --prefix` put the library
  somewhere else, sends every user's build to the wrong place. Hence
  `pcfiledir`.

- **Changing a public enum.** Reordering `SIG_LOWPASS` and `SIG_HIGHPASS`
  compiles cleanly everywhere, and silently swaps the two filters in every
  program built against the old header and run with the new library. Values in
  a public header never change within a major version; new ones go at the end.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Build `analyse.c` with CMake against the installed `sig`, using
   `find_package`. Then ask for `sig` 2.0, and 0.9.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-find-package/`:

   ```bash
   cat CMakeLists.txt
   ```

   ```cmake
   # A project that uses an INSTALLED sig, found by find_package: no copy
   # of sig's source, and no add_subdirectory.
   cmake_minimum_required(VERSION 3.16)
   project(analyse LANGUAGES C)

   # The version this project needs; -DSIG_WANT=... asks for another.
   set(SIG_WANT 1.0 CACHE STRING "The version of sig to ask for")
   find_package(sig ${SIG_WANT} REQUIRED)

   add_executable(analyse ../../analyse.c)
   target_link_libraries(analyse PRIVATE sig::sig)
   ```

   `CMAKE_PREFIX_PATH` adds a prefix for `find_package` to search, as
   `PKG_CONFIG_PATH` does for pkg-config:

   ```bash
   cmake -S . -B build -DCMAKE_PREFIX_PATH="$PWD/../../staging"
   cmake --build build
   ```

   ```bash
   build/analyse
   ```

   ```
   sig 1.0.0, installed
     750.00 Hz  0.485
    6000.00 Hz  0.028
   ```

   `find_package(sig 1.0)` read `staging/lib/cmake/sig/sigConfig.cmake`, which
   defines the target `sig::sig`, with its include folder, its library and its
   location. CMake also records that location as the program's rpath, so no
   `-Wl,-rpath` was needed. Other versions:

   ```bash
   for v in 2.0 0.9; do
     if cmake -S . -B build-$v -DCMAKE_PREFIX_PATH="$PWD/../../staging" -DSIG_WANT=$v > /dev/null 2>&1; then
       echo "sig $v: found"
     else
       echo "sig $v: not found"
     fi
   done
   ```

   ```
   sig 2.0: not found
   sig 0.9: not found
   ```

   `sigConfigVersion.cmake` was written with `COMPATIBILITY SameMajorVersion`:
   1.0.0 satisfies a request for 1.0 or any 1.x below it, and nothing from
   another major version, in either direction. The same rule as the soname.

   </details>

2. Move the whole install somewhere else. Do the tools and pkg-config still
   work?

   <details><summary>Output and answer</summary>

   This one uses the lesson's own folder. `staging` is moved, not copied, so
   nothing can be found at the old place, and moved back at the end:

   ```bash
   mv staging moved
   moved/bin/sig-tone 1 750 | moved/bin/sig-fft
   PKG_CONFIG_PATH=moved/lib/pkgconfig pkg-config --cflags sig
   mv moved staging
   ```

   ```
     750.00 Hz  0.500
   -Imoved/lib/pkgconfig/../../include
   ```

   Both work, because neither has a path written in. The tools find `libsig` at
   `$ORIGIN/../lib`, where `$ORIGIN` is the folder the program is in, and
   `sig.pc` finds the headers relative to itself. A relocatable install can be
   packaged as an archive and unpacked anywhere, which is how most binary
   packages, Python wheels included (lesson 35), carry a C library.

   </details>

3. Which of `sig`'s functions can a program find in the shared library at run
   time? Ask for `sig_version` and `sig__freq_ok`.

   <details><summary>Output and answer</summary>

   This one uses the lesson's own folder. Python's `ctypes` (lesson 35) opens a
   shared library and looks up names in it, the way the loader does. The loop
   picks whichever file this system made:

   ```bash
   for lib in staging/lib/libsig.so staging/lib/libsig.dylib; do
     [ -e "$lib" ] && python3 -c "import ctypes, sys; lib = ctypes.CDLL(sys.argv[1]); print('sig_version:', hasattr(lib, 'sig_version')); print('sig__freq_ok:', hasattr(lib, 'sig__freq_ok'))" "$lib"
   done
   ```

   ```
   sig_version: True
   sig__freq_ok: False
   ```

   `sig__freq_ok` is in the library, and `osc.c` and `biquad.c` call it, but
   hidden: inside the library it is an ordinary function, and outside it
   doesn't exist. In lesson 29's static `libsig.a` it was visible to every
   program, because an archive is just object files, and visibility is a
   property of shared libraries. With GNU binutils, `nm -D` lists what a shared
   library exports: the 13 `SIG_API` functions, and nothing else.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                             | header      | what it does                 |
| ---------------------------------------------------- | ----------- | ---------------------------- |
| [`printf`](https://en.cppreference.com/c/io/fprintf) | `<stdio.h>` | formatted output to `stdout` |

<!-- /libfuncs -->
