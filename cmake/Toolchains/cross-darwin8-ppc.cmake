# Note cross-compiling only works if CMake is patched.  See patch at:
#   http://public.kitware.com/Bug/view.php?id=14603
#
set(CMAKE_SYSTEM_NAME      "Darwin")
set(XCOMPILE True)

set(ENV{MACOSX_DEPLOYMENT_TARGET} 10.4)
set(ENV{CMAKE_OSX_ARCHITECTURES} ppc)

set(TARGET "powerpc-apple-darwin8")

# COMPILER_HOME should also be the location of the SDK
set(COMPILER_HOME "/opt/${TARGET}")

set(CMAKE_OSX_SYSROOT "${COMPILER_HOME}" CACHE STRING "Toolchain CMAKE_OSX_SYSROOT")
set(CMAKE_OSX_ARCHITECTURES "ppc" CACHE STRING "Toolchain CMAKE_OSX_ARCHITECTURES")

set(CMAKE_C_COMPILER "${COMPILER_HOME}/usr/bin/${TARGET}-gcc")
set(CMAKE_CXX_COMPILER "${COMPILER_HOME}/usr/bin/${TARGET}-g++")

set(CMAKE_FIND_ROOT_PATH "${COMPILER_HOME}" "${CMAKE_SOURCE_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)