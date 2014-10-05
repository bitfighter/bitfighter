# We must use the LuaJIT internal build system
include(ExternalProject)

set(LUAJIT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/lua/luajit/src")
set(LUAJIT_LIBRARY "libluajit.a")

set(LUAJIT_BUILDCOMMAND "make" "amalg")
set(LUAJIT_POSTBUILDCOMMAND "")

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
	
	# CMAKE_OSX_ARCHITECTURES is i386 or ppc
	elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "i386")
		set(CC "${CMAKE_C_COMPILER} -arch i386 -isysroot ${CMAKE_OSX_SYSROOT}")
		set(LUAJIT_BUILDCOMMAND "make" "CC=${CC}" "amalg")
	
	elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "ppc")
		# LuaJIT requires GCC >= 4.3 for ppc.  OSX 10.4 only provides 4.0 so
		# this will fail to compile.  LuaJIT is turned off for PPC...
		set(CC "${CMAKE_C_COMPILER} -arch ppc -isysroot ${CMAKE_OSX_SYSROOT}")
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
