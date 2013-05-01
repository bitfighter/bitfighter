# Finds Speex library
#
#  SPEEX_INCLUDE_DIR - where to find speex.h, etc.
#  SPEEX_LIBRARIES   - List of libraries when using Speex.
#  SPEEX_FOUND       - True if Speex found.

set(SPEEX_SEARCH_PATHS
	${SPEEX_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

if(SPEEX_INCLUDE_DIR)
	# Already in cache, be silent
	set(SPEEX_FIND_QUIETLY TRUE)
endif()


find_path(SPEEX_INCLUDE_DIR 
	NAMES speex/speex.h
	HINTS ENV SPEEXDIR
	PATH_SUFFIXES include include/speex speex
	PATHS ${SPEEX_SEARCH_PATHS}
)

find_library(SPEEX_LIBRARY NAMES 
	NAMES speex libspeex
	HINTS ENV SPEEXDIR
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	PATHS ${SPEEX_SEARCH_PATHS}
)


if(SPEEX_LIBRARY)
	set(SPEEX_LIBRARIES ${SPEEX_LIBRARY})
else()
	set(SPEEX_LIBRARIES)
endif()


# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SPEEX DEFAULT_MSG SPEEX_LIBRARIES SPEEX_INCLUDE_DIR)
