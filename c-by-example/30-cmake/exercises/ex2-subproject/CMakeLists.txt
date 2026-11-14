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
