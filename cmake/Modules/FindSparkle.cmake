# Finds Sparkle library
#
#  SPARKLE_INCLUDE_DIR - where to find Sparkle.h, etc.
#  SPARKLE_LIBRARIES   - List of libraries when using Sparkle.
#  SPARKLE_FOUND       - True if Sparkle found.

set(SPARKLE_SEARCH_PATHS
	${SPARKLE_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
)

if(SPARKLE_INCLUDE_DIR)
	# Already in cache, be silent
	set(SPARKLE_FIND_QUIETLY TRUE)
endif()


find_path(SPARKLE_INCLUDE_DIR 
	NAMES Sparkle.h
	PATHS ${SPARKLE_SEARCH_PATHS}
)

find_library(SPARKLE_LIBRARY NAMES 
	NAMES Sparkle
	PATHS ${SPARKLE_SEARCH_PATHS}
)


if(SPARKLE_LIBRARY)
	set(SPARKLE_LIBRARIES ${SPARKLE_LIBRARY})
else()
	set(SPARKLE_LIBRARIES)
endif()


# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sparkle DEFAULT_MSG SPARKLE_LIBRARIES SPARKLE_INCLUDE_DIR)

