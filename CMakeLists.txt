project (bitfighter)
cmake_minimum_required (VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")


# No creating project files in-source.  Bad, evil things happen
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" _insource)
if (_insource)
	message(SEND_ERROR "No building in-source.  You must run CMake from the \"build\" directory.")
	message(FATAL_ERROR "Remove the file CMakeCache.txt in ${CMAKE_SOURCE_DIR} first.")
endif()

# Default to Release for a build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()


#
# Environment verification
#
if(APPLE)
	set(OSX_DEPLOY_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})
	
	message(STATUS "MACOSX_DEPLOYMENT_TARGET: ${OSX_DEPLOY_TARGET}")
	# MACOSX_DEPLOYMENT_TARGET must be set in the environment to compile properly
	if(NOT OSX_DEPLOY_TARGET)
		message(FATAL_ERROR "MACOSX_DEPLOYMENT_TARGET environment variable not set.  Set this like so: 'export MACOSX_DEPLOYMENT_TARGET=10.6'")
	endif()
endif()


#
# Global linker flags
# 
if(MSVC)
	# Using the following NODEFAULTLIB to fix LNK4098 warning and some linker errors
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG          ${CMAKE_EXE_LINKER_FLAGS_DEBUG}          "/NODEFAULTLIB:libc.lib;libcmt.lib;msvcrt.lib;libcd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE        ${CMAKE_EXE_LINKER_FLAGS_RELEASE}        "/NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} "/NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     ${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     "/NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")

	# Disable incremental linking on RelWithDebInfo for sam686
	set(LinkerFlags
		CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
	)
	
	foreach(LinkerFlag ${LinkerFlags})
		# Handle both /INCREMENTAL:YES and /INCREMENTAL
		string(REPLACE "INCREMENTAL:YES" "INCREMENTAL" ${LinkerFlag} "${${LinkerFlag}}")
		string(REPLACE "INCREMENTAL" "INCREMENTAL:NO" ${LinkerFlag} "${${LinkerFlag}}")
	endforeach()
else()
	# MinGW won't statically compile in Microsofts c/c++ library routines
	if(MINGW)
		set(BF_LINK_FLAGS "-Wl,--as-needed -static-libgcc")
	elseif(NOT APPLE)
		set(BF_LINK_FLAGS "-Wl,--as-needed")
	endif()
	
	# Only link in what is absolutely necessary
	set(CMAKE_EXE_LINKER_FLAGS ${BF_LINK_FLAGS})
endif()


# 
# Global compiler specific flags
# 
if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
endif()

if(MSVC)
	# Using /MT avoids linking against the stupid MSVC runtime libraries
	set(CompilerFlags
		CMAKE_CXX_FLAGS
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
	)
	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
	
	# Enable 'Edit and Continue' debugging support
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
endif()

if(MINGW)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
endif()


#
# Determine game versions.  Used in various places later
#
file(STRINGS "${CMAKE_SOURCE_DIR}/zap/version.h" VERSION_H_FILE)
string(REGEX REPLACE ".*ZAP_GAME_RELEASE[ ]+\"([^\"]+)\".*" "\\1" BITFIGHTER_RELEASE "${VERSION_H_FILE}")
message(STATUS "Bitfighter Release: ${BITFIGHTER_RELEASE}")

string(REGEX REPLACE ".*#define BUILD_VERSION[ ]+([^ ]+).*" "\\1" BITFIGHTER_BUILD_VERSION_KEY "${VERSION_H_FILE}")
string(REGEX REPLACE ".*${BITFIGHTER_BUILD_VERSION_KEY}[ ]+([0-9]+).*" "\\1" BITFIGHTER_BUILD_VERSION "${VERSION_H_FILE}")
message(STATUS "Bitfighter Build Version: ${BITFIGHTER_BUILD_VERSION}")


#
# CMake options
#

# Default to NO for Linux
option(USE_SDL2 "Force usage of SDL2" NO)

option(USE_GLES "Force usage of OpenGL ES for the bitfighter client.  Requires SDL2." NO)
option(USE_LUAJIT "Use LuaJIT." YES)

#
# Now search for dependencies
#
# Set up Windows or OSX specific locations for libraries and headers
#
if(WIN32 OR APPLE OR XCOMPILE)
	# Always use SDL2 on OSX or Windows
	set(USE_SDL2 1)

	set(SDL2_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libsdl)
	set(OGG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libogg)
	set(VORBIS_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
	set(VORBISFILE_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
	set(SPEEX_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libspeex)
	set(MODPLUG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libmodplug)
	set(ALURE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/alure)

	# OpenAL
	set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openal/include")
	if(APPLE)
		set(OPENAL_LIBRARY "${CMAKE_SOURCE_DIR}/lib/OpenAL-Soft.framework")
	else()
		set(OPENAL_LIBRARY "${CMAKE_SOURCE_DIR}/lib/OpenAL32.lib")
	endif()

	# libpng
	set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
	set(PNG_PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
	if(APPLE)
		set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/lib/libpng.framework")
	else()
		set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/lib/libpng14.lib")
	endif()
	
	# zlib
	set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
	if(NOT APPLE)
		set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/lib/zlib.lib")
	endif()
endif()

if(MSVC)
	# Add parallel build to Visual Studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

	# Disable deprecated warnings (fopen, vsnprintf), some alternative function may not exist on other compilers including gcc.
	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
endif()


# Search for our dependencies
# SDL2 is default and should be found on all platforms except possibly Linux
if(USE_SDL2)
	find_package(SDL2)

	if(SDL2_FOUND)
		set(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
		set(SDL_LIBRARY ${SDL2_LIBRARY})
	endif()
else()
	find_package(SDL)
	
	# SDL_LIBRARY and SDL_INCLUDE_DIR are set by the FindSDL module
endif()

if(WIN32)
	find_package(ALURE)
else()
	set(ALURE_LIBRARIES alure)
endif()

find_package(OpenAL)

# Searches for opengl and gnables gles if gl isnt found in the system.
message(STATUS "Determining system GL provider")
if(NOT USE_GLES)
	message(STATUS "Atempting to use standard OpenGL")
	find_package(OpenGL)
	set(USE_STANDARD_GL 1)
	set(GL_LIBRARY ${OPENGL_LIBRARY})
	set(GL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR})
endif()

if(USE_GLES OR NOT USE_STANDARD_GL)
	message(STATUS "Atempting to use OpenGL ES")
	add_definitions(-DBF_USE_GLES)
	add_definitions(-DBF_NO_CONSOLE)
	find_package(OpenGLES)
	set(GL_LIBRARY ${OPENGLES_LIBRARY})
	set(GL_INCLUDE_DIR ${OPENGLES_INCLUDE_DIR})
endif()

find_package(Threads REQUIRED)
find_package(PNG)
find_package(OGG)
find_package(Vorbis)
if(APPLE)
	set(SPARKLE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib)
	find_package(Sparkle)
	# OSX doesn't use vorbisfile (or it's built-in to normal vorbis, I think)
	set(VORBISFILE_LIBRARIES "")
else()
	find_package(VorbisFile)
endif()
find_package(Speex)
find_package(ModPlug)
find_package(MySQL)


# Disable certain sub-projects based on dependency search
set(COMPILE_ALURE 1)
set(COMPILE_ZAP 1)

if(NOT OPENAL_FOUND OR NOT VORBIS_FOUND OR NOT MODPLUG_FOUND)
	set(COMPILE_ALURE 0)
endif()

if( NOT COMPILE_ALURE OR (NOT SDL2_FOUND AND NOT SDL_FOUND) )
	set(COMPILE_ZAP 0)
endif()


# Add our sub-projects
add_subdirectory(tnl)

if(USE_LUAJIT)
	add_subdirectory(lua/luajit)
	include_directories(${CMAKE_SOURCE_DIR}/lua/luajit/src)
else()
	add_subdirectory(lua/lua-vec)
	include_directories(${CMAKE_SOURCE_DIR}/lua/lua-vec/src)
endif()

if(NOT WIN32 AND COMPILE_ALURE)  # ALURE is not compiled on Windows presently
	add_subdirectory(alure)
endif()
add_subdirectory(mysql++)
add_subdirectory(libtomcrypt)
add_subdirectory(master)
if(COMPILE_ZAP)
	add_subdirectory(zap)
	add_subdirectory(gtest)
endif()
