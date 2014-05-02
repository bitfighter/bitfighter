include("${CMAKE_SOURCE_DIR}/cmake/Platform/Shared.cmake")
set(CMAKE_EXE_LINKER_FLAGS "/NODEFAULTLIB:MSVCRT.lib")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /NODEFAULTLIB:libc.lib;libcmt.lib;msvcrt.lib;libcd.lib;msvcrtd.lib")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")

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

# Disable incremental linking on RelWithDebInfo for sam686
set(LinkerFlags
	CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
)

foreach(LinkerFlag ${LinkerFlags})
	# Handle both /INCREMENTAL:YES and /INCREMENTAL
	string(REPLACE "INCREMENTAL:YES" "INCREMENTAL" ${LinkerFlag} "${${LinkerFlag}}")
	string(REPLACE "INCREMENTAL" "INCREMENTAL:NO" ${LinkerFlag} "${${LinkerFlag}}")
endforeach()

if(NOT XCOMPILE)
	# Add icon resource in Visual Studio
	list(APPEND CLIENT_SOURCES ZAP.rc)
endif()

set(EXTRA_LIBS ws2_32 winmm)

# Work around the "Debug", "Release", etc. directories Visual Studio tries to add
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	set_target_properties(test bitfighterd bitfighter 
		PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/exe
	)
endforeach()

# set search paths
BF_SET_SEARCH_PATHS()
set(OPENAL_LIBRARY "${CMAKE_SOURCE_DIR}/lib/OpenAL32.lib")
set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/lib/zlib.lib")
set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/lib/libpng14.lib")

find_package(VorbisFile)

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

# Install windows libraries and resources
# The trailing slash is necessary to do here for proper native path translation
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resource/ resDir)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lib/ libDir)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lua/luajit/src/ luaLibDir)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)
		
if(WIN32)
	if(MSYS OR CYGWIN OR XCOMPILE)
		set(RES_COPY_CMD cp -r ${resDir}* ${exeDir})
		set(LIB_COPY_CMD cp ${libDir}*.dll ${exeDir})
	else()
		set(RES_COPY_CMD xcopy /e /d /y ${resDir}* ${exeDir})
		set(LIB_COPY_CMD xcopy /d /y ${libDir}*.dll ${exeDir})
	endif()
endif()

BF_COPY_RESOURCES()