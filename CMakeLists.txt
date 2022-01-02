project (bitfighter)
cmake_minimum_required (VERSION 3.1.0)

cmake_policy(SET CMP0043 OLD)

#
# CMake options
#

option(USE_LEGACY_GL "Use legacy OpenGL (OpenGL 1.0) instead of modern OpenGL." NO)
option(USE_GLES "Force usage of OpenGL ES for the bitfighter client.  Requires SDL2." NO)
option(ALURE_DISABLE_MP3 "Disable dynamic loading of libmpg123.  Disables mp3 completely." NO)
option(NO_THREADS "Disable usage of threads in TNL.  May cause issues." NO)
option(LUAJIT_BUILTIN "Use built-in LuaJIT.  Recommended." YES)
option(DISCORD "Integrate Discord RPC calls." YES)


#
# CMake system management
#

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Require use c++11 compiler
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use compiler extensions like -std=gnu++11
# set(CMAKE_CXX_EXTENSIONS OFF)

# Use appropriate platform alterations
if(WIN32)
	include(${CMAKE_SOURCE_DIR}/cmake/Platform/Win32.cmake)
elseif(APPLE)
	include(${CMAKE_SOURCE_DIR}/cmake/Platform/Apple.cmake)
elseif(UNIX)
	include(${CMAKE_SOURCE_DIR}/cmake/Platform/Linux.cmake)
endif()


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
# Library searching and dependencies
#

# SDL2 is default and should be found on all platforms except possibly Linux
find_package(SDL2)

if(SDL2_FOUND)
	set(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
	set(SDL_LIBRARY ${SDL2_LIBRARY})
endif()

# Searches for OpenGL and enables GLES if GL isnt found in the system
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
	set(USE_GLES 1)
	find_package(OpenGLES)
	set(GL_LIBRARY ${OPENGLES_LIBRARY})
	set(GL_INCLUDE_DIR ${OPENGLES_INCLUDE_DIR})
endif()

if(USE_LEGACY_GL)
	add_definitions(-DBF_USE_LEGACY_GL)
endif()

# Other needed libraries that don't have in-tree fallback options
if(NOT NO_THREADS)
	find_package(Threads REQUIRED)
endif()
find_package(PNG)
find_package(MySQL)
find_package(OGG)
find_package(Speex)
find_package(Vorbis)
find_package(ModPlug)
find_package(OpenAL)

# Add OpenGL loader if we aren't using legacy OpenGL
if(NOT USE_LEGACY_GL)
	add_subdirectory(glad)
	set(GL_LOADER_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/glad/include")
	set(GL_LOADER_LIBRARY glad)
	add_definitions(-DGL_MAJOR_VERSION=2)
	add_definitions(-DGL_MINOR_VERSION=1)
endif()

# Now look for libraries that have an in-tree fallback option
# Many of these may have special overrides to properly adapt to the in-tree versions

# Special function to look for a system lib, but fall back to the in-tree version
function(FIND_WITH_FALLBACK packageName capsName dirName)
	if(NOT FORCE_IN_TREE_${capsName})
		find_package(${packageName})
	endif()

	if( NOT ${capsName}_FOUND )
		set(${capsName}_LIBRARIES ${dirName})
		set(${capsName}_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/${dirName}")
	endif()

	# export the results
	set(${capsName}_FOUND "${${capsName}_FOUND}" PARENT_SCOPE)
	set(${capsName}_INCLUDE_DIR "${${capsName}_INCLUDE_DIR}" PARENT_SCOPE)
	set(${capsName}_LIBRARY "${${capsName}_LIBRARY}" PARENT_SCOPE)
	set(${capsName}_LIBRARIES "${${capsName}_LIBRARIES}" PARENT_SCOPE)
endfunction()


# ALURE
find_with_fallback(ALURE ALURE alure)
if(NOT ALURE_FOUND)
	if(NOT OPENAL_FOUND OR NOT VORBIS_FOUND OR NOT MODPLUG_FOUND)
		message(WARNING "Audio system components are missing.  Bitfighter will be compiled without audio")
		add_definitions(-DBF_NO_AUDIO)
		set(OPENAL_INCLUDE_DIR "")
	else()
		message(STATUS "Using in-tree ALURE")
		add_definitions(-DALURE_STATIC_LIBRARY)
		add_subdirectory(alure)
	endif()
endif()


# sqlite3
find_with_fallback(Sqlite SQLITE3 sqlite)
if(NOT SQLITE3_FOUND)
	message(STATUS "Using in-tree sqlite3")
	add_subdirectory(sqlite)
endif()


# libtomcrypt
find_with_fallback(TomCrypt TOMCRYPT tomcrypt)
if(NOT TOMCRYPT_FOUND)
	message(STATUS "Using in-tree libtomcrypt")
	set(TOMCRYPT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/tomcrypt/src/headers")
	add_subdirectory(tomcrypt)
endif()


# Boost
find_with_fallback(Boost Boost boost)
if(NOT Boost_FOUND)
	message(STATUS "Using in-tree Boost c++ headers")
	set(BOOST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/boost")
endif()


# Clipper
find_with_fallback(Clipper CLIPPER clipper)
if(NOT CLIPPER_FOUND)
	message(STATUS "Using in-tree Clipper")
	add_subdirectory(clipper)
endif()


# poly2tri
find_with_fallback(Poly2tri POLY2TRI poly2tri)
if(NOT POLY2TRI_FOUND)
	message(STATUS "Using in-tree poly2tri")
	add_subdirectory(poly2tri)
endif()


# LuaJIT / Lua
if(NOT LUAJIT_BUILTIN)
find_with_fallback(LuaJit LUAJIT lua)
endif()

if(LUAJIT_FOUND)
	set(LUA_LIB ${LUAJIT_LIBRARIES})
	set(LUA_INCLUDE_DIR ${LUAJIT_INCLUDE_DIR})
else()
	message(STATUS "Using in-tree LuaJIT")
	# Use internal LuaJIT
	set(LUA_LIB luajit)
	set(LUA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lua/luajit/src")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/lua/${LUA_LIB}")
	add_subdirectory("${CMAKE_SOURCE_DIR}/lua/${LUA_LIB}")
endif()

# Discord
if(DISCORD)
	set(BUILD_EXAMPLES OFF CACHE BOOL "Build example apps")
	
	add_subdirectory(discord-rpc)
	
	set(DISCORD_LIBRARY discord-rpc)
	add_definitions(-DBF_DISCORD)
endif()


# mysql++ is for compiling master
if(EXISTS "${CMAKE_SOURCE_DIR}/mysql++")
	add_subdirectory(mysql++)
endif()


# gtest is for compiling the bitfighter_test suite
if(EXISTS "${CMAKE_SOURCE_DIR}/gtest")
	add_subdirectory(gtest EXCLUDE_FROM_ALL)
endif()


# Other internal-only sub-projects
add_subdirectory(tnl)
add_subdirectory(tnlping)
add_subdirectory(master)
add_subdirectory(updater)  # Windows-only


# Our main sources!
add_subdirectory(zap)
