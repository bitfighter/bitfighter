# Note cross-compiling only works if CMake is patched.  See patch at:
#   http://public.kitware.com/Bug/view.php?id=14603
#
set(CMAKE_SYSTEM_NAME      "Darwin")
set(XCOMPILE True)

set(ENV{MACOSX_DEPLOYMENT_TARGET} 10.6)

# Architecture isn't needed in cross-compiling x86_64.  It passes in the '-arch' flag
# which breaks apple's gcc 4.2.1
# set(ENV{CMAKE_OSX_ARCHITECTURES} x86_64)

set(TARGET "x86_64-apple-darwin10")

# COMPILER_HOME should also be the location of the SDK
set(COMPILER_HOME "/opt/${TARGET}")

set(CMAKE_OSX_SYSROOT "${COMPILER_HOME}" CACHE STRING "Toolchain CMAKE_OSX_SYSROOT")
# set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Toolchain CMAKE_OSX_ARCHITECTURES")

# Specify the cross compiler
set(CMAKE_C_COMPILER "${COMPILER_HOME}/usr/bin/${TARGET}-gcc")
set(CMAKE_CXX_COMPILER "${COMPILER_HOME}/usr/bin/${TARGET}-g++")

set(CMAKE_FIND_ROOT_PATH "${COMPILER_HOME}" "${CMAKE_SOURCE_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
