# - Find modplug
# Find the native modplug includes and libraries
#
#  MODPLUG_INCLUDE_DIR - where to find modplug.h, etc.
#  MODPLUG_LIBRARIES   - List of libraries when using libmodplug.
#  MODPLUG_FOUND       - True if modplug found.

set(MODPLUG_SEARCH_PATHS
	${MODPLUG_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)


find_path(MODPLUG_INCLUDE_DIR 
	NAMES libmodplug/modplug.h
	HINTS ENV MODPLUGDIR
	PATH_SUFFIXES include include/libmodplug
	PATHS ${MODPLUG_SEARCH_PATHS}
)

find_library(MODPLUG_LIBRARY
	NAMES modplug libmodplug
	HINTS ENV MODPLUGDIR
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	PATHS ${MODPLUG_SEARCH_PATHS}
)


if(MODPLUG_LIBRARY)
	set(MODPLUG_LIBRARIES ${MODPLUG_LIBRARY})
else()
	set(MODPLUG_LIBRARIES)
endif()


# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MODPLUG DEFAULT_MSG MODPLUG_LIBRARIES MODPLUG_INCLUDE_DIR)
