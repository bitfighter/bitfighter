# Finds Clipper library
#
#  CLIPPER_INCLUDE_DIR - where to find clipper.hpp, etc.
#  CLIPPER_LIBRARIES   - List of libraries when using Clipper.
#  CLIPPER_FOUND       - True if Clipper found.

set(CLIPPER_SEARCH_PATHS
	${CLIPPER_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

if(CLIPPER_INCLUDE_DIR)
	# Already in cache, be silent
	set(CLIPPER_FIND_QUIETLY TRUE)
endif()


find_path(CLIPPER_INCLUDE_DIR 
	NAMES clipper.hpp
	HINTS ENV CLIPPERDIR
	PATH_SUFFIXES include include/polyclipping include/clipper clipper
	PATHS ${CLIPPER_SEARCH_PATHS}
)

find_library(CLIPPER_LIBRARY NAMES 
	NAMES polyclipping clipper
	HINTS ENV CLIPPERDIR
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	PATHS ${CLIPPER_SEARCH_PATHS}
)


if(CLIPPER_LIBRARY)
	set(CLIPPER_LIBRARIES ${CLIPPER_LIBRARY})
else()
	set(CLIPPER_LIBRARIES)
endif()


# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CLIPPER DEFAULT_MSG CLIPPER_LIBRARIES CLIPPER_INCLUDE_DIR)
