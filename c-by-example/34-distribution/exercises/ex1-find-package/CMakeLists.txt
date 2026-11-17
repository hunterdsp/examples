# A project that uses an INSTALLED sig, found by find_package: no copy
# of sig's source, and no add_subdirectory.
cmake_minimum_required(VERSION 3.16)
project(analyze LANGUAGES C)

# The version this project needs; -DSIG_WANT=... asks for another.
set(SIG_WANT 1.0 CACHE STRING "The version of sig to ask for")
find_package(sig ${SIG_WANT} REQUIRED)

add_executable(analyze ../../analyze.c)
target_link_libraries(analyze PRIVATE sig::sig)
