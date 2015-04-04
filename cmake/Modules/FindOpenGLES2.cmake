# - Try to find OpenGL ES 2

set(GLES2_SEARCH_PATHS
	${GLES2_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
	/opt/graphics/OpenGL
	/opt/vc  # Raspberry pi
	/usr/openwin
	/usr/shlib
	/usr/X11R6/
)


find_path(OPENGLES2_INCLUDE_DIR 
	NAMES GLES2/gl2.h OpenGLES/ES2/gl.h
	PATH_SUFFIXES include
	PATHS ${GLES2_SEARCH_PATHS}
)

find_library(OPENGLES2_LIBRARY 
	NAMES libGLESv2 GLESv2
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64 lib/x86_64-linux-gnu/mesa-egl
	PATHS ${GLES2_SEARCH_PATHS}
)

if(OPENGLES2_LIBRARY)
	set(OPENGLES2_LIBRARIES ${OPENGLES2_LIBRARY})
else()
	message(ERROR "OpenGL ES 2 Library not found! debug FindOpenGLES2.cmake!")
endif()

# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OPENGLES2 DEFAULT_MSG OPENGLES2_LIBRARIES OPENGLES2_INCLUDE_DIR)
