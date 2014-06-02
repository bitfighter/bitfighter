# the name of the target operating system
set(CMAKE_SYSTEM_NAME "Windows")
# set(WIN32 True)
set(MINGW True)
set(XCOMPILE True)

# Specify the cross-compiler
# We use -m32 because bitfighter is 32bit-only on Windows at the moment
set(CMAKE_C_COMPILER "i686-w64-mingw32-gcc")
set(CMAKE_CXX_COMPILER "i686-w64-mingw32-g++")
set(CMAKE_RC_COMPILER "i686-w64-mingw32-windres")

set(CMAKE_C_FLAGS "-m32" CACHE STRING "Toolchain CMAKE_C_FLAGS")
set(CMAKE_CXX_FLAGS "-m32" CACHE STRING "Toolchain CMAKE_CXX_FLAGS")

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
