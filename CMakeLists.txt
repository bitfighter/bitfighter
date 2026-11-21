cmake_minimum_required (VERSION 3.10)		# Updated Nov 2025
project (bitfighter)

enable_testing()

# cmake_policy(VERSION 3.27)
# cmake_policy(SET CMP0177 NEW)

#
# CMake options
#

option(USE_LEGACY_GL "Use legacy OpenGL (OpenGL 1.0) instead of modern OpenGL." NO)
option(USE_GLES "Force usage of OpenGL ES for the bitfighter client.  Requires SDL2." NO)
option(ALURE_DISABLE_MP3 "Disable dynamic loading of libmpg123.  Disables mp3 completely." NO)
option(NO_THREADS "Disable usage of threads in TNL.  May cause issues." NO)
option(LUAJIT_BUILTIN "Use built-in LuaJIT.  Recommended." YES)
option(DISCORD "Integrate Discord RPC calls." YES)

# Browser limitations. Native builds leave the options above alone.
if(EMSCRIPTEN)
	# LuaJIT's interpreter is generated machine code. There is no wasm backend
	# in the vendored tree, so the Emscripten build uses PUC Lua 5.1 instead.
	set(LUAJIT_BUILTIN OFF)
	# pthreads need SharedArrayBuffer and cross-origin isolation. The PoC runs
	# TNL threads inline (see TNL_NO_THREADS).
	set(NO_THREADS ON)
	# Discord's RPC socket has no browser transport.
	set(DISCORD OFF)
endif()


#
# CMake system management
#

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Require use c++17 compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use compiler extensions like -std=gnu++11
# set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Prevent ambiguity between Windows SDK's ::byte and C++17's std::byte
    add_compile_definitions(_HAS_STD_BYTE=0)
endif()

# Use appropriate platform alterations.
# EMSCRIPTEN is also UNIX, so it has to be tested first. Linux.cmake's
# --as-needed link flag is rejected by wasm-ld.
if(EMSCRIPTEN)
	include(${CMAKE_SOURCE_DIR}/cmake/Platform/Emscripten.cmake)
elseif(WIN32)
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

# SDL2 is default and should be found on all platforms except possibly Linux.
# Emscripten supplies it as a port (-sUSE_SDL=2), not as a CMake package.
if(EMSCRIPTEN)
	set(SDL2_FOUND TRUE)
	set(SDL_INCLUDE_DIR "")
	set(SDL_LIBRARY "")
else()
	find_package(SDL2)

	if(SDL2_FOUND)
		set(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
		set(SDL_LIBRARY ${SDL2_LIBRARY})
	endif()
endif()

# Searches for OpenGL and enables GLES if GL isn't found in the system
message(STATUS "Determining system GL provider")
if(EMSCRIPTEN)
	# WebGL is OpenGL ES 2.0, linked by -sFULL_ES2=1. There is no host libGL.
	message(STATUS "Using Emscripten WebGL (OpenGL ES 2.0)")
	set(USE_GLES 1)
	add_definitions(-DBF_USE_GLES)
	set(GL_LIBRARY "")
	set(GL_INCLUDE_DIR "")
elseif(NOT USE_GLES)
	message(STATUS "Attempting to use standard OpenGL")
	find_package(OpenGL)
	set(USE_STANDARD_GL 1)
	set(GL_LIBRARY ${OPENGL_LIBRARY})
	set(GL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR})
endif()

if(NOT EMSCRIPTEN AND (USE_GLES OR NOT USE_STANDARD_GL))
	message(STATUS "Attempting to use OpenGL ES")
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
	add_definitions(-DBF_GL_MAJOR_VERSION=2)
	add_definitions(-DBF_GL_MINOR_VERSION=1)
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
if(EMSCRIPTEN)
	# OpenAL is the WebAudio port. Vorbis/Ogg and ModPlug are Emscripten
	# ports (-sUSE_VORBIS, -sUSE_MODPLUG). Speex has no port; it is fetched
	# below. ALURE decodes wav, ogg, and (via the port) tracker modules.
	message(STATUS "Emscripten audio: OpenAL + ALURE + Vorbis + ModPlug + Speex")
	include(FetchContent)
	FetchContent_Declare(speex
		URL https://downloads.xiph.org/releases/speex/speex-1.2.1.tar.gz
		URL_HASH SHA256=4b44d4f2b38a370a2d98a78329fefc56a0cf93d1c1be70029217baae6628feea
	)
	FetchContent_GetProperties(speex)
	if(NOT speex_POPULATED)
		FetchContent_Populate(speex)
	endif()
	configure_file(
		${CMAKE_SOURCE_DIR}/cmake/emscripten/speex/speex_config_types.h
		${speex_SOURCE_DIR}/include/speex/speex_config_types.h
		COPYONLY
	)
	set(SPEEX_SRC_DIR ${speex_SOURCE_DIR}/libspeex)
	set(SPEEX_SOURCES
		${SPEEX_SRC_DIR}/cb_search.c
		${SPEEX_SRC_DIR}/exc_10_32_table.c
		${SPEEX_SRC_DIR}/exc_8_128_table.c
		${SPEEX_SRC_DIR}/filters.c
		${SPEEX_SRC_DIR}/gain_table.c
		${SPEEX_SRC_DIR}/hexc_table.c
		${SPEEX_SRC_DIR}/high_lsp_tables.c
		${SPEEX_SRC_DIR}/lsp.c
		${SPEEX_SRC_DIR}/ltp.c
		${SPEEX_SRC_DIR}/speex.c
		${SPEEX_SRC_DIR}/stereo.c
		${SPEEX_SRC_DIR}/vbr.c
		${SPEEX_SRC_DIR}/vq.c
		${SPEEX_SRC_DIR}/bits.c
		${SPEEX_SRC_DIR}/exc_10_16_table.c
		${SPEEX_SRC_DIR}/exc_20_32_table.c
		${SPEEX_SRC_DIR}/exc_5_256_table.c
		${SPEEX_SRC_DIR}/exc_5_64_table.c
		${SPEEX_SRC_DIR}/gain_table_lbr.c
		${SPEEX_SRC_DIR}/hexc_10_32_table.c
		${SPEEX_SRC_DIR}/lpc.c
		${SPEEX_SRC_DIR}/lsp_tables_nb.c
		${SPEEX_SRC_DIR}/modes.c
		${SPEEX_SRC_DIR}/modes_wb.c
		${SPEEX_SRC_DIR}/nb_celp.c
		${SPEEX_SRC_DIR}/quant_lsp.c
		${SPEEX_SRC_DIR}/sb_celp.c
		${SPEEX_SRC_DIR}/speex_callbacks.c
		${SPEEX_SRC_DIR}/speex_header.c
		${SPEEX_SRC_DIR}/window.c
	)
	add_library(speex STATIC ${SPEEX_SOURCES})
	target_include_directories(speex
		PUBLIC ${speex_SOURCE_DIR}/include
		PRIVATE ${CMAKE_SOURCE_DIR}/cmake/emscripten/speex ${SPEEX_SRC_DIR}
	)
	target_compile_definitions(speex PRIVATE HAVE_CONFIG_H FLOATING_POINT)
	set(SPEEX_LIBRARIES speex)
	set(SPEEX_INCLUDE_DIR "${speex_SOURCE_DIR}/include")

	set(OPENAL_FOUND TRUE)
	set(OPENAL_LIBRARY "")
	set(OPENAL_INCLUDE_DIR "${EMSCRIPTEN_ROOT_PATH}/system/include/AL")
	set(PNG_INCLUDE_DIR "")
	set(PNG_LIBRARY "")
	# The port is linked via -sUSE_MODPLUG. ALURE calls it; no extra cmake lib.
	set(MODPLUG_LIBRARIES "")
	set(OGG_LIBRARIES "")
	set(VORBIS_LIBRARIES "")
	set(VORBISFILE_LIBRARIES "")
	# FALSE so the client target depends on the alure library built below.
	set(ALURE_FOUND FALSE)
	add_subdirectory(alure)
	set(ALURE_LIBRARIES alure)
	set(ALURE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/alure/include")
else()
	find_with_fallback(ALURE ALURE alure)
	if(NOT ALURE_FOUND)
		if(NOT OPENAL_FOUND)
			message(WARNING "OpenAL not found.  Bitfighter will be compiled without audio")
			add_definitions(-DBF_NO_AUDIO)
			set(OPENAL_INCLUDE_DIR "")
		else()
			message(STATUS "Using in-tree ALURE")
			add_subdirectory(alure)
			set(ALURE_LIBRARIES alure)
			set(ALURE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/alure/include")
		endif()
	endif()
endif()


# sqlite3
find_with_fallback(Sqlite SQLITE3 sqlite)
if(NOT SQLITE3_FOUND)
	message(STATUS "Using in-tree sqlite3")
	add_subdirectory(sqlite)
endif()


# libtommath (in-tree)
add_subdirectory(libtommath)

# libtomcrypt
find_with_fallback(TomCrypt TOMCRYPT libtomcrypt)
if(NOT TOMCRYPT_FOUND)
	message(STATUS "Using in-tree libtomcrypt")
	set(TOMCRYPT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libtomcrypt/src/headers")
	set(TOMCRYPT_LIBRARIES tomcrypt)
	add_subdirectory(libtomcrypt)
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

# Clipper2
find_with_fallback(Clipper2 CLIPPER2 Clipper2Lib)
if(NOT CLIPPER2_FOUND)
	message(STATUS "Using in-tree Clipper2")
	add_subdirectory(Clipper2Lib)
	# find_with_fallback sets LIBRARIES to the dirname string; override
	# with the actual CMake target name so linking works correctly.
	set(CLIPPER2_LIBRARIES clipper2)
	# Headers are under include/, not at the root of the directory.
	set(CLIPPER2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Clipper2Lib/include")
endif()


# poly2tri
find_with_fallback(Poly2tri POLY2TRI poly2tri)
if(NOT POLY2TRI_FOUND)
	message(STATUS "Using in-tree poly2tri")
	add_subdirectory(poly2tri)
endif()


# LuaJIT / Lua
#
# The in-tree LuaJIT is 2.0.5, which predates Apple Silicon and cannot target
# arm64 macOS.  Require a system LuaJIT 2.1 (e.g. `brew install luajit`) there.
# CMAKE_OSX_ARCHITECTURES may be a list (e.g. a universal "arm64;x86_64"), so test
# for membership rather than equality -- the bundled LuaJIT can't build an arm64
# slice in that case either.
if(APPLE AND LUAJIT_BUILTIN AND "arm64" IN_LIST CMAKE_OSX_ARCHITECTURES)
	message(FATAL_ERROR
		"The bundled LuaJIT 2.0.5 cannot be built for arm64 macOS.\n"
		"Install LuaJIT 2.1 and configure with the system copy:\n"
		"    brew install luajit\n"
		"    cmake .. -DLUAJIT_BUILTIN=NO")
endif()

if(EMSCRIPTEN)
	# LuaJIT has no WebAssembly backend. Fetch the official Lua 5.1.5
	# tarball at configure time instead of vendoring it.
	if(CMAKE_VERSION VERSION_LESS 3.14)
		message(FATAL_ERROR "The Emscripten build downloads Lua 5.1.5 and needs CMake 3.14 or newer.")
	endif()
	include(FetchContent)
	FetchContent_Declare(lua51
		URL https://www.lua.org/ftp/lua-5.1.5.tar.gz
		URL_HASH SHA256=2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333
	)
	FetchContent_GetProperties(lua51)
	if(NOT lua51_POPULATED)
		FetchContent_Populate(lua51)
	endif()
	file(GLOB LUA51_SOURCES "${lua51_SOURCE_DIR}/src/*.c")
	list(REMOVE_ITEM LUA51_SOURCES
		"${lua51_SOURCE_DIR}/src/lua.c"
		"${lua51_SOURCE_DIR}/src/luac.c")
	add_library(lua STATIC ${LUA51_SOURCES})
	target_include_directories(lua PUBLIC "${lua51_SOURCE_DIR}/src")
	message(STATUS "Using fetched Lua 5.1.5 (${lua51_SOURCE_DIR})")
	set(LUA_LIB lua)
	set(LUA_INCLUDE_DIR "${lua51_SOURCE_DIR}/src")
else()
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


# gtest is for compiling the bitfighter_test suite.
# Skip it on Emscripten; the suite is a native target and gtest's pthreads
# setup fights the single-threaded browser build.
if(EXISTS "${CMAKE_SOURCE_DIR}/gtest" AND NOT EMSCRIPTEN)
	add_subdirectory(gtest EXCLUDE_FROM_ALL)
endif()


# Other internal-only sub-projects
include_directories(tnl)
add_subdirectory(tnl)
add_subdirectory(tnlping)
add_subdirectory(master)
add_subdirectory(updater)  # Windows-only


# Our main sources!
add_subdirectory(zap)
