# Finds ALURE library
#
#  ALURE_INCLUDE_DIR - where to find alure.h, etc.
#  ALURE_LIBRARIES   - List of libraries when using ALURE.
#  ALURE_FOUND       - True if ALURE found.

set(ALURE_SEARCH_PATHS
	${ALURE_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

if(ALURE_INCLUDE_DIR)
	# Already in cache, be silent
	set(ALURE_FIND_QUIETLY TRUE)
endif()


find_path(ALURE_INCLUDE_DIR 
	NAMES alure.h
	HINTS ENV ALUREDIR
	PATH_SUFFIXES include include/alure AL alure
	PATHS ${ALURE_SEARCH_PATHS}
)

find_library(ALURE_LIBRARY NAMES 
	NAMES alure libalure alure32 libalure32
	HINTS ENV ALUREDIR
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	PATHS ${ALURE_SEARCH_PATHS}
)


if(ALURE_LIBRARY)
	set(ALURE_LIBRARIES ${ALURE_LIBRARY})
else()
	set(ALURE_LIBRARIES)
endif()


# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ALURE DEFAULT_MSG ALURE_LIBRARIES ALURE_INCLUDE_DIR)
