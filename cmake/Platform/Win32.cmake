include("${CMAKE_SOURCE_DIR}/cmake/Platform/Shared.cmake")

## Global project configuration

#
# Linker flags
# 
if(MSVC)
	# Using the following NODEFAULTLIB to fix LNK4098 warning and some linker errors
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /NODEFAULTLIB:libc.lib;libcmt.lib;msvcrt.lib;libcd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")

	# Disable incremental linking on RelWithDebInfo for sam686
	set(LinkerFlags
		CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
	)
	
	foreach(LinkerFlag ${LinkerFlags})
		# Handle both /INCREMENTAL:YES and /INCREMENTAL
		# .. First set everything to '/INCREMENTAL', then turn it off
		string(REPLACE "INCREMENTAL:YES" "INCREMENTAL" ${LinkerFlag} "${${LinkerFlag}}")
		string(REPLACE "INCREMENTAL" "INCREMENTAL:NO" ${LinkerFlag} "${${LinkerFlag}}")
	endforeach()
endif()

if(MINGW)
	# MinGW won't statically compile in Microsofts c/c++ library routines
	set(BF_LINK_FLAGS "-Wl,--as-needed -static-libgcc")
	
	# Only link in what is absolutely necessary
	set(CMAKE_EXE_LINKER_FLAGS ${BF_LINK_FLAGS})
endif()


# 
# Compiler specific flags
# 
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
	
	# Add parallel build to Visual Studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

	# Disable deprecated warnings (fopen, vsnprintf), some alternative function may not exist on other compilers including gcc.
	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
endif()

if(MINGW)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
endif()


#
# Library searching and dependencies
#

# From Shared.cmake
#SHARED_SET_LIBRARY_SEARCH_PATHS()

# Always use SDL2 on OSX or Windows
set(USE_SDL2 YES)
set(USE_LUAJIT YES)

# Set some search paths
set(SDL2_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libsdl)
set(OGG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libogg)
set(VORBIS_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
set(VORBISFILE_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
set(SPEEX_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libspeex)
set(MODPLUG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libmodplug)
set(ALURE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/alure)

# Directly set include dirs for some libraries
set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openal/include")
set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
# libpng needs two for some weird reason
set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
set(PNG_PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")

# Directly specify some libs (because of deficiences in CMake modules?)
set(OPENAL_LIBRARY "${CMAKE_SOURCE_DIR}/lib/OpenAL32.lib")
set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/lib/zlib.lib")
set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/lib/libpng14.lib")
	
find_package(VorbisFile)


## End Global project configuration


## Sub-project configuration
function(BF_PLATFORM_SET_EXTRA_SOURCES)
	if(NOT XCOMPILE)
		# Add icon resource in Visual Studio
		list(APPEND CLIENT_SOURCES ZAP.rc)
	endif()
endfunction()


function(BF_PLATFORM_SET_EXTRA_LIBS)
	set(EXTRA_LIBS ws2_32 winmm PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_APPEND_LIBS)
	# Do nothing!
endfunction()


function(BF_PLATFORM_ADD_DEFINITIONS)
	if(NOT MSVC)
		add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)
	endif()
endfunction()


function(BF_PLATFORM_SET_TARGET_PROPERTIES)
	if(MSVC)
		# Work around the "Debug", "Release", etc. directories Visual Studio tries to add
		foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
			set_target_properties(test bitfighterd bitfighter 
				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/exe
			)
		endforeach()
		
		# Separate output name "bitfighter_debug.exe" for debug build, to avoid conflicts with debug/release build
		set_target_properties(test bitfighterd bitfighter PROPERTIES DEBUG_POSTFIX "_debug")

		# Set some linker flags to use console mode in debug build, etc..
		# Always use SUBSYSTEM:CONSOLE; hiding the console window is controlled in zap/main.cpp near the bottom of main()
		# Allows console to stay visible if ran from typing in command window.
		set_target_properties(test bitfighterd bitfighter PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(test bitfighterd bitfighter PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(test bitfighterd bitfighter PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
		set_target_properties(test bitfighterd bitfighter PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
		
		# Set more compiler flags for console on appropriate targets
		list(APPEND ALL_DEBUG_DEFS "_CONSOLE")
	endif()
endfunction()


function(BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES)
	# The trailing slash is necessary to do here for proper native path translation
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resource/ resDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lib/ libDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lua/luajit/src/ luaLibDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)
	
	# Set copy command
	if(MSYS OR CYGWIN OR XCOMPILE)
		set(RES_COPY_CMD cp -r ${resDir}* ${exeDir})
		set(LIB_COPY_CMD cp ${libDir}*.dll ${exeDir})
	else()
		set(RES_COPY_CMD xcopy /e /d /y ${resDir}* ${exeDir})
		set(LIB_COPY_CMD xcopy /d /y ${libDir}*.dll ${exeDir})
	endif()
	
	# Copy resources
	add_custom_command(TARGET test bitfighterd bitfighter POST_BUILD 
		COMMAND ${RES_COPY_CMD}
		COMMAND ${LIB_COPY_CMD}
	)
endfunction()


function(BF_PLATFORM_INSTALL)
	# Do nothing!
endfunction()


function(BF_PLATFORM_CREATE_PACKAGES)
	# Do nothing!
endfunction()
