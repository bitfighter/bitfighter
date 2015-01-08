# We must use the LuaJIT internal build system
include(ExternalProject)

# Options
option(LUAJIT_DISABLE_JIT "Disable the JIT compiler in LuaJIT.  Try this if LuaJIT doesn't compile for your platform" NO)


set(LUAJIT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/lua/luajit/src")
set(LUAJIT_LIBRARY "libluajit.a")

set(LUAJIT_BUILDCOMMAND "make" "amalg")
set(LUAJIT_POSTBUILDCOMMAND "")

if(LUAJIT_DISABLE_JIT)
	message(STATUS "LuaJIT: JIT compiler will be disabled")
	set(CC "${CMAKE_C_COMPILER} -DLUAJIT_DISABLE_JIT")
	set(LUAJIT_BUILDCOMMAND "make" "CC=${CC}" "amalg")
endif()

# Different library names depending on platform
if(MSVC)
	set(LUAJIT_BUILDCOMMAND "msvcbuild.bat" "staticamalg")
	
	# Use the .lib for linking, later we'll copy the DLL
	set(LUAJIT_LIBRARY "lua51.lib")
	
elseif(APPLE)
	# OSX 64 bit must use shared lib (otherwise game crashes)
	# The library is created with extension '.so' so we'll rename it to .dylib
	# to conform with OSX standards
	if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
		set(LUAJIT_LIBRARY "libluajit.dylib")
		
		# Modify the library's internal id name to play nicely with our bundle
		set(LUAJIT_POSTBUILDCOMMAND cp ${LUAJIT_SOURCE_DIR}/libluajit.so ${LUAJIT_SOURCE_DIR}/${LUAJIT_LIBRARY} 
			COMMAND install_name_tool -id @loader_path/../Frameworks/libluajit.dylib ${LUAJIT_SOURCE_DIR}/libluajit.dylib
		)
	endif()
	
	# CMAKE_OSX_ARCHITECTURES is i386 or ppc
	if(CMAKE_OSX_ARCHITECTURES STREQUAL "i386")
		set(CC "${CMAKE_C_COMPILER} -arch i386 -mmacosx-version-min=10.4 -isysroot ${CMAKE_OSX_SYSROOT}")
		set(LUAJIT_BUILDCOMMAND "make" "CC=${CC}" "amalg")
	
	elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "ppc")
		# LuaJIT can be compiled for OSX PPC, but the JIT VM must be disabled.  This leaves us with
		# the normal interpreter (which is still faster than standard Lua)
		set(CC "${CMAKE_C_COMPILER} -arch ppc -mmacosx-version-min=10.4 -isysroot ${CMAKE_OSX_SYSROOT} -DLUAJIT_DISABLE_JIT")
		set(LUAJIT_BUILDCOMMAND "make" "CC=${CC}" "amalg")
	endif()
endif()


ExternalProject_Add(luajit_external
	SOURCE_DIR ${LUAJIT_SOURCE_DIR}
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE 1
	BUILD_COMMAND ${LUAJIT_BUILDCOMMAND}
	INSTALL_COMMAND ""
)


add_custom_command(TARGET luajit_external POST_BUILD
	COMMAND ${LUAJIT_POSTBUILDCOMMAND}
)

add_library(luajit STATIC IMPORTED GLOBAL)  # GLOBAL so it is visible everywhere
set_property(TARGET luajit PROPERTY IMPORTED_LOCATION ${LUAJIT_SOURCE_DIR}/${LUAJIT_LIBRARY})
add_dependencies(luajit luajit_external)

include_directories(${LUAJIT_SOURCE_DIR})
