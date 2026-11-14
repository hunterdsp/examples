# 30 · CMake

Run this lesson's commands in `c-by-example/30-cmake/`.

Lesson 29 built `sig` by hand: four compiles, an `ar`, and a link, each with
the right flags. Lesson 08's Makefile would remember those commands, but only
for one compiler on one kind of system. `-fvisibility=hidden` is a GCC and
clang flag, a shared library is `libsig.so` on Linux, `libsig.dylib` on macOS
and `sig.dll` on Windows, and Visual Studio wants none of it as a Makefile.

**CMake** describes the build once, as *targets* (this library, built from
these files; that program, linked with this library), and writes the real build
files for whatever system it runs on: Makefiles, Ninja files, a Visual Studio
solution, an Xcode project. Most C libraries you will meet are built with it,
and anything that wants to use them expects it. `sig`'s description:

```bash
cat ../sig/CMakeLists.txt
```

```cmake
# sig -- a small signal-processing library, built by CMake.
#
#   cmake -S . -B build             configure: find a compiler, write the
#                                   build files into build/
#   cmake --build build             compile the library, tools and tests
#   ctest --test-dir build          run the tests
#   cmake --install build --prefix DIR
#                                   install it under DIR
#
# Options, set with -DNAME=VALUE when configuring:
#
#   SIG_SANITIZE=ON        build with AddressSanitizer and UBSan
#   BUILD_SHARED_LIBS=ON   a shared library instead of a static one
#   SIG_BUILD_TESTS=OFF    skip the tests
#   SIG_BUILD_TOOLS=OFF    skip sig-tone, sig-filter and sig-fft
cmake_minimum_required(VERSION 3.16)

# The version is written once, in include/sig/common.h, and read from it
# here, so the header and the build can't disagree.
file(STRINGS include/sig/common.h version_lines
     REGEX "^#define SIG_VERSION_(MAJOR|MINOR|PATCH) ")
foreach(line IN LISTS version_lines)
  string(REGEX MATCH "SIG_VERSION_([A-Z]+) ([0-9]+)" _ "${line}")
  set(SIG_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
endforeach()

project(sig VERSION ${SIG_MAJOR}.${SIG_MINOR}.${SIG_PATCH} LANGUAGES C)

option(SIG_SANITIZE "Build with AddressSanitizer and UBSan" OFF)
option(BUILD_SHARED_LIBS "Build a shared library instead of a static one"
       OFF)
option(SIG_BUILD_TESTS "Build the tests" ON)
option(SIG_BUILD_TOOLS "Build sig-tone, sig-filter and sig-fft" ON)

# C99, exactly: -std=c99, not GNU's dialect.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# build/compile_commands.json: every compile command, for editors.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra) # for everything in this file
endif()

include(GNUInstallDirs)
# An installed tool finds a shared libsig in ../lib, relative to itself,
# wherever the whole tree was installed (lesson 34).
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

# The library. Its public headers are in include/; src/ is private.
add_library(sig src/common.c src/osc.c src/biquad.c src/fft.c)
add_library(sig::sig ALIAS sig)
target_include_directories(
  sig
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE src)
if(UNIX)
  target_link_libraries(sig PRIVATE m) # sin, cos, cexp: the maths library
endif()
# A sanitized library needs sanitized programs (lesson 29), so the flags
# are PUBLIC: whatever links sig gets them too. Only in this build tree,
# never in an installed sig (lesson 34).
if(SIG_SANITIZE)
  target_compile_options(
    sig PUBLIC $<BUILD_INTERFACE:-fsanitize=address,undefined>)
  target_link_options(sig PUBLIC
                      $<BUILD_INTERFACE:-fsanitize=address,undefined>)
endif()
# A shared library shows only what SIG_API marks (lesson 34), and is
# named for its version: libsig.so.1.0.0, linked as libsig.so.1.
set_target_properties(
  sig PROPERTIES C_VISIBILITY_PRESET hidden VERSION ${PROJECT_VERSION}
                 SOVERSION ${PROJECT_VERSION_MAJOR})

if(SIG_BUILD_TOOLS)
  foreach(tool sig-tone sig-filter sig-fft)
    add_executable(${tool} tools/${tool}.c)
    target_link_libraries(${tool} PRIVATE sig)
  endforeach()
endif()

if(SIG_BUILD_TESTS)
  enable_testing()
  foreach(name common osc biquad fft)
    add_executable(test_${name} tests/test_${name}.c)
    target_link_libraries(test_${name} PRIVATE sig $<$<BOOL:${UNIX}>:m>)
    add_test(NAME ${name} COMMAND test_${name})
  endforeach()
  # The tools, end to end: two tones, through a low-pass, into the FFT.
  if(SIG_BUILD_TOOLS)
    add_test(NAME pipeline
             COMMAND sh -c "$<TARGET_FILE:sig-tone> 1 750 6000 \
| $<TARGET_FILE:sig-filter> lowpass 1500 | $<TARGET_FILE:sig-fft>")
    set_tests_properties(
      pipeline PROPERTIES PASS_REGULAR_EXPRESSION
                          "750.00 Hz  0.485\n 6000.00 Hz  0.028\n$")
  endif()
endif()

# Documentation from the comments in include/, if Doxygen is installed:
# cmake --build build --target docs.
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  configure_file(docs/Doxyfile.in Doxyfile @ONLY)
  add_custom_target(
    docs
    COMMAND Doxygen::doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Writing the documentation into ${CMAKE_CURRENT_BINARY_DIR}/docs"
    VERBATIM)
endif()

# Installing: the library, its headers, the tools, and two ways for
# another project to find them, a pkg-config file and a CMake package.
install(
  TARGETS sig
  EXPORT sigTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/sig DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(SIG_BUILD_TOOLS)
  install(TARGETS sig-tone sig-filter sig-fft
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

configure_file(sig.pc.in sig.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sig.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

include(CMakePackageConfigHelpers)
set(cmakedir ${CMAKE_INSTALL_LIBDIR}/cmake/sig)
install(
  EXPORT sigTargets
  NAMESPACE sig::
  DESTINATION ${cmakedir})
configure_package_config_file(cmake/sigConfig.cmake.in sigConfig.cmake
                              INSTALL_DESTINATION ${cmakedir})
write_basic_package_version_file(sigConfigVersion.cmake
                                 COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sigConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/sigConfigVersion.cmake
        DESTINATION ${cmakedir})
```

The second half, tests, documentation and installing, is lessons 31, 32 and 34.
This lesson is the first half.

## Configure, then build

Building takes two steps. **Configuring** runs `CMakeLists.txt`: CMake finds a
compiler, checks what it can do, and writes the build files into a folder of
their own, `-B build` (`-S` is where the source is). Options are set here, with
`-D`:

```bash
cmake -S ../sig -B build -DSIG_SANITIZE=ON
```

It prints what it found, which differs from machine to machine. **Building**
runs the build files it wrote. `cmake --build` works whatever kind they are, so
the same command builds with `make`, Ninja or Visual Studio:

```bash
cmake --build build
```

Everything it made is in `build/`, and nothing was added to `../sig`. The tools
are there, ready to use:

```bash
build/sig-tone 1 750 6000 | build/sig-fft
```

```
  750.00 Hz  0.500
 6000.00 Hz  0.500
```

Edit a file and run `cmake --build build` again, and only what depends on it is
rebuilt: the same bookkeeping as lesson 08's `make`, done for you.

## What it shows

- **Targets, not commands.** `add_library(sig src/common.c …)` says what the
  library is made of, and nothing about how to compile it. CMake knows how for
  every compiler it supports, and picks `ar` and `libsig.a`, or whatever this
  system uses.
- **Usage requirements.**
  `target_include_directories(sig PUBLIC … include PRIVATE src)` says that
  compiling `sig` needs both folders, and compiling *anything that links `sig`*
  needs `include/`. So `sig-tone` gets `-I …/include` from
  `target_link_libraries(sig-tone PRIVATE sig)`, without asking, and never gets
  `src/`: `internal.h` stays private. `PUBLIC` means "me and my users",
  `PRIVATE` "only me", and `INTERFACE` "only my users".
- **The sanitizers travel the same way.** `SIG_SANITIZE` adds them to `sig` as
  `PUBLIC` options, so every program linked with it is sanitized too, and
  lesson 29's undefined `__asan_` functions can't happen.
- **Options.** `option(SIG_SANITIZE …)` declares a switch, off unless
  `-DSIG_SANITIZE=ON`. `BUILD_SHARED_LIBS` is CMake's own: every `add_library`
  without a kind makes a shared library when it is on (lesson 34).
- **C99, exactly.** `CMAKE_C_STANDARD 99` with `CMAKE_C_EXTENSIONS OFF` becomes
  `-std=c99`, the flag this course has used throughout, and not `-std=gnu99`,
  which accepts GNU's additions without a word.
- **One version.** The version is in `common.h` as three `#define`s, and
  `file(STRINGS …)` reads it from there, so the header and the build always
  agree. It ends up in the library's file name and its installed files (lesson
  34).
- **`compile_commands.json`.** `CMAKE_EXPORT_COMPILE_COMMANDS` writes every
  compile command into `build/compile_commands.json`, which is how editors and
  `clangd` know your `-I` folders. (`../sig/compile_flags.txt` does the same
  job more simply, for `sig`'s own folder.)

## Pitfalls

- **No build type, no optimisation.** With Makefiles or Ninja, a build
  configured without `-DCMAKE_BUILD_TYPE` is compiled with no `-O` at all
  (exercise 3). It runs, slowly, and any timing taken from it measures nothing
  that ships (lesson 19). Choose one: `Debug` (`-g`), `Release`
  (`-O3 -DNDEBUG`) or `RelWithDebInfo` (`-O2 -g -DNDEBUG`).

- **The cache remembers.** Every `-D` is stored in `build/CMakeCache.txt`, and
  stays set for every later configure of that folder. Changing an option's
  default in `CMakeLists.txt` doesn't change a folder that already has a value
  for it. When in doubt, delete the build folder: it holds nothing that can't
  be made again.

- **Building inside the source.** `cmake -S . -B .`, or plain `cmake .` in the
  source folder, writes the build files among the sources, and there is no
  clean way to get rid of them. Always give the build a folder of its own.

- **Globbing for sources.** `file(GLOB …)` can collect every `.c` file in a
  folder, but it runs only when CMake configures, so a new file is silently
  left out of the build until someone configures again. `sig` names its files.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Configure a second build of `sig`, in `build-lib`, with neither tests nor
   tools. What does it build?

   <details><summary>Output and answer</summary>

   This one uses the lesson's own folder.

   ```bash
   cmake -S ../sig -B build-lib -DSIG_BUILD_TESTS=OFF -DSIG_BUILD_TOOLS=OFF
   cmake --build build-lib
   ```

   ```bash
   test -e build-lib/sig-tone && echo "has sig-tone" || echo "no sig-tone"
   test -e build/sig-tone && echo "has sig-tone" || echo "no sig-tone"
   ```

   ```
   no sig-tone
   has sig-tone
   ```

   Only the library. One source folder can have any number of build folders,
   each with its own options and its own compiler, and none of them touches the
   source or each other: a sanitized build, a release build and a clang build
   of the same code, side by side.

   </details>

2. Build a program of your own together with `sig`, as part of one project,
   without installing anything.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-subproject/`:

   ```bash
   cat CMakeLists.txt
   ```

   ```cmake
   # A project of its own that builds sig as part of itself: sig's source
   # folder is added like one of this project's own, and its targets are
   # then this project's too.
   cmake_minimum_required(VERSION 3.16)
   project(beep LANGUAGES C)

   # sig's tests and tools aren't this project's business. Setting the
   # variables before add_subdirectory answers sig's option()s for it.
   set(SIG_BUILD_TESTS OFF)
   set(SIG_BUILD_TOOLS OFF)
   # A folder outside this one needs a second argument: where, inside this
   # build, sig's build files go.
   add_subdirectory(../../../sig sig)

   add_executable(beep beep.c)
   target_link_libraries(beep PRIVATE sig::sig)
   ```

   ```bash
   cmake -S . -B build -DSIG_SANITIZE=ON
   cmake --build build
   ```

   ```bash
   build/beep
   ```

   ```
   sig 1.0.0: bin 128 has amplitude 1.000
   ```

   `add_subdirectory` runs `sig`'s `CMakeLists.txt` as part of this project, so
   `sig::sig` is a target here like any other, and
   [`beep.c`](exercises/ex2-subproject/beep.c) gets its include folder and its
   sanitizer flags from it. This is how a project *vendors* a library: a copy
   of its source inside the project, or a git submodule, built every time, with
   the project's own compiler and options. `sig::sig` is an *alias* for `sig`;
   the `::` can only be a target, never a file, so a typo is an error, not a
   search for a library called `sig::sig`.

   </details>

3. Configure `sig` with `-DCMAKE_BUILD_TYPE=Release` in `build-release`. How
   many of the compile commands in each build optimise?

   <details><summary>Output and answer</summary>

   This one uses the lesson's own folder. `grep -c` counts the lines that
   match, and `compile_commands.json` has one `"command"` line per compile:

   ```bash
   cmake -S ../sig -B build-release -DCMAKE_BUILD_TYPE=Release
   ```

   ```bash
   grep -c -- '-O' build/compile_commands.json
   grep -c -- '-O3 -DNDEBUG' build-release/compile_commands.json
   ```

   ```
   0
   11
   ```

   The first build set no type, and not one of its 11 compiles optimises. A
   build type is a set of flags, and `Release` adds `-O3 -DNDEBUG` to every
   compile: optimisation, and `NDEBUG`, which deletes every `assert` (lesson 31
   has something to say about that). Configuring is enough to write
   `compile_commands.json`: nothing had to be built to read it.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                             | header      | what it does                 |
| ---------------------------------------------------- | ----------- | ---------------------------- |
| [`printf`](https://en.cppreference.com/c/io/fprintf) | `<stdio.h>` | formatted output to `stdout` |

<!-- /libfuncs -->
