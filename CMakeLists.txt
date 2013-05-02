project (bitfighter)
cmake_minimum_required (VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# Linker flags
if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "/NODEFAULTLIB:MSVCRT.lib /NODEFAULTLIB:LIBCMT.lib")
else()
	# Only link in what is absolutely necessary
	set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")
endif()

# Compiler specific flags
if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wno-narrowing")
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
endif()

if(MSVC)
	# Using /MT avoids linking against the stupid MSVC runtime libraries
	set(CompilerFlags
		CMAKE_CXX_FLAGS
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_C_FLAGS
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_RELEASE
	)
	foreach(CompilerFlag ${CompilerFlags})
	  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
endif()


# Define the Linux data dir if not defined in a packaging build script already
if("${CMAKE_SYSTEM}" MATCHES "Linux")
	if(NOT LINUX_DATA_DIR)
		set(LINUX_DATA_DIR "/usr/share")
	endif()

	message(STATUS "LINUX_DATA_DIR:  " ${LINUX_DATA_DIR})

	# Quotes need to be a part of the definition or the compiler won't understand
	add_definitions(-DLINUX_DATA_DIR="${LINUX_DATA_DIR}")
endif()


# Now search for dependencies
# Set up Windows specific locations for libraries and headers
if(WIN32)
	set(SDL2_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libsdl)
	set(OGG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libogg)
	set(VORBIS_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
	set(VORBISFILE_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
	set(SPEEX_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libspeex)
	set(MODPLUG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libmodplug)
	set(ALURE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/alure)

	# OpenAL
	set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openal/include")
	set(OPENAL_LIBRARY "${CMAKE_SOURCE_DIR}/lib/OpenAL32.lib")
	
	# libpng
	set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
	set(PNG_PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
	set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/lib/libpng14.lib")
	
	# zlib
	set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
	set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/lib/zlib.lib")
endif()

if(MSVC)
	# Add parallel build to Visual Studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# ALURE is not compiled on visual studio presently
if(NOT MSVC)
	add_subdirectory(alure)
endif()
add_subdirectory(tnl)
add_subdirectory(lua/lua-vec)
add_subdirectory(mysql++)
add_subdirectory(libtomcrypt)
add_subdirectory(master)
add_subdirectory(gtest)
add_subdirectory(zap)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" _insource)
if (_insource)
	message(SEND_ERROR "You must run \"cmake ..\" from the \"build\" directory")
	message(FATAL_ERROR "Remove the file CMakeCache.txt in ${CMAKE_SOURCE_DIR} first.")
endif (_insource)
