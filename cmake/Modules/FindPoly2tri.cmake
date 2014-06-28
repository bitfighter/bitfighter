# Finds poly2tri library
#
#  POLY2TRI_INCLUDE_DIR - where to find poly2tri.h, etc.
#  POLY2TRI_LIBRARIES   - List of libraries when using poly2tri.
#  POLY2TRI_FOUND       - True if poly2tri found.

set(POLY2TRI_SEARCH_PATHS
	${POLY2TRI_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

if(POLY2TRI_INCLUDE_DIR)
	# Already in cache, be silent
	set(POLY2TRI_FIND_QUIETLY TRUE)
endif()


find_path(POLY2TRI_INCLUDE_DIR 
	NAMES poly2tri.h
	HINTS ENV CMAKE_CURRENT_SOURCE_DIRDIR
	PATH_SUFFIXES include include/poly2tri poly2tri
	PATHS ${POLY2TRI_SEARCH_PATHS}
)

find_library(POLY2TRI_LIBRARY NAMES 
	NAMES poly2tri
	HINTS ENV POLY2TRIDIR
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	PATHS ${POLY2TRI_SEARCH_PATHS}
)


if(POLY2TRI_LIBRARY)
	set(POLY2TRI_LIBRARIES ${POLY2TRI_LIBRARY})
else()
	set(POLY2TRI_LIBRARIES)
endif()


# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(POLY2TRI DEFAULT_MSG POLY2TRI_LIBRARIES POLY2TRI_INCLUDE_DIR)
