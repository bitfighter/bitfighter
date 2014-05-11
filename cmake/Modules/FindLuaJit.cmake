# Finds LuaJit library
#
#  LUAJIT_INCLUDE_DIR - where to find lua.h, etc.
#  LUAJIT_LIBRARIES   - List of libraries when using luajit.
#  LUAJIT_FOUND       - True if luajit found.

set(LUAJIT_SEARCH_PATHS
	${LUAJIT_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)


find_path(LUAJIT_INCLUDE_DIR 
	NAMES lua.h
	HINTS ENV LUAJITDIR
	PATH_SUFFIXES include include/luajit luajit luajit-2.0 luajit/src
	PATHS ${LUAJIT_SEARCH_PATHS}
)

find_library(LUAJIT_LIBRARIES NAMES 
	NAMES luajit libluajit luajit luajit-5.1
	HINTS ENV LUAJITDIR
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	PATHS ${LUAJIT_SEARCH_PATHS}
)

# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LUAJIT DEFAULT_MSG LUAJIT_LIBRARIES LUAJIT_INCLUDE_DIR)
