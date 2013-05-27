# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Darwin)
SET(APPLE True)

SET(TARGET i686-apple-darwin11 CACHE STRING "Cross compiler triplet")
SET(OSX_SDK_VER MacOSX10.6 CACHE STRING "OSX SDK version to build against")
SET(GCC_VER 4.2.1 CACHE STRING "Cross environment gcc version")

SET(_CMAKE_TOOLCHAIN_LOCATION /usr/${TARGET}/usr/bin)

SET(CMAKE_OSX_SYSROOT "/usr/${TARGET}" CACHE STRING "xchain target root")

SET(CMAKE_C_FLAGS "-I/usr/${TARGET}/usr/include -I/usr/${TARGET}/usr/lib/gcc/${TARGET}/${GCC_VER}/include/" CACHE STRING "")

SET(CMAKE_CXX_FLAGS "-I/usr/${TARGET}/usr/include -I/usr/${TARGET}/usr/lib/gcc/${TARGET}/${GCC_VER}/include/ -I/usr/${TARGET}/SDKs/${OSX_SDK_VER}.sdk/usr/include/c++/${GCC_VER} -I/usr/${TARGET}/SDKs/${OSX_SDK_VER}.sdk/usr/include/c++/${GCC_VER}/i686-apple-darwin10/" CACHE STRING "")

SET(CMAKE_C_COMPILER ${TARGET}-gcc)
SET(CMAKE_CXX_COMPILER ${TARGET}-g++)
SET(CMAKE_LINKER ${TARGET}-ld64)
SET(CMAKE_CXX_COMPILER_ENV_VAR CXX)

# search for libs and include files in the cross environment
SET(CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR} /usr/${TARGET})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
