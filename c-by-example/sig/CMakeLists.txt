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
