project (bitfighter)
cmake_minimum_required (VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS_DEBUG "-g")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
endif()


# Define the Linux data dir if not defined in a packaging build script already
if("${CMAKE_SYSTEM}" MATCHES "Linux")
	if(NOT LINUX_DATA_DIR)
		set(LINUX_DATA_DIR "/usr/share")
	endif()

	message(STATUS "LINUX_DATA_DIR:  " ${LINUX_DATA_DIR})

	# Quotes need to be a part of the definition or the compiler won't understand
	add_definitions(-DLINUX_DATA_DIR="\\"${LINUX_DATA_DIR}\\"")
endif()


add_subdirectory(alure)
add_subdirectory(tnl)
add_subdirectory(lua/lua-vec)
add_subdirectory(mysql++)
add_subdirectory(libtomcrypt)
add_subdirectory(master)
add_subdirectory(zap)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" _insource)
if (_insource)
	message(SEND_ERROR "You must run \"cmake ..\" from the \"build\" directory")
	message(FATAL_ERROR "Remove the file CMakeCache.txt in ${CMAKE_SOURCE_DIR} first.")
endif (_insource)
