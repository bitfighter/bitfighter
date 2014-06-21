## Global project configuration

#
# Linker flags
# 
set(BF_CLIENT_LIBRARY_BEFORE_FLAGS "-Wl,-whole-archive")
set(BF_CLIENT_LIBRARY_AFTER_FLAGS "-Wl,-no-whole-archive")

set(BF_LINK_FLAGS "-Wl,--as-needed")

# Only link in what is absolutely necessary
set(CMAKE_EXE_LINKER_FLAGS ${BF_LINK_FLAGS})

# 
# Compiler specific flags
# 
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

# Define the Linux data dir if not defined in a packaging build script already
if("${CMAKE_SYSTEM}" MATCHES "Linux")
	if(NOT LINUX_DATA_DIR)
		set(LINUX_DATA_DIR "/usr/share")
	endif()

	message(STATUS "LINUX_DATA_DIR: ${LINUX_DATA_DIR}.  Change this by invoking cmake with -DLINUX_DATA_DIR=<SOME_DIRECTORY>")

	# Quotes need to be a part of the definition or the compiler won't understand
	add_definitions(-DLINUX_DATA_DIR="${LINUX_DATA_DIR}")
endif()


#
# Library searching and dependencies
# 
find_package(VorbisFile)

## End Global project configuration


## Sub-project configuration
#
# Note that any variable adjustment from the parent CMakeLists.txt will
# need to be re-set with the PARENT_SCOPE option

function(BF_PLATFORM_SET_EXTRA_SOURCES)
	# Do nothing!
endfunction()


function(BF_PLATFORM_SET_EXTRA_LIBS)
	set(EXTRA_LIBS dl m PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_APPEND_LIBS)
	# Do nothing!
endfunction()


function(BF_PLATFORM_ADD_DEFINITIONS)
	add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)
endfunction()


function(BF_PLATFORM_SET_TARGET_PROPERTIES targetName)
	# Do nothing!
endfunction()


function(BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES targetName)
	# Do nothing!
endfunction()


function(BF_PLATFORM_INSTALL targetName)
	install(TARGETS ${targetName} RUNTIME DESTINATION bin)
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/
		DESTINATION share/games/bitfighter/
	)
endfunction()


function(BF_PLATFORM_CREATE_PACKAGES targetName)
	# Do nothing!
endfunction()
