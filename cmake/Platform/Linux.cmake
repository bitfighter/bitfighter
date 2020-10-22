
option(INSTALL_NOTIFIER "Install the bitfighter notifier" YES)

## Global project configuration

#
# Linker flags
# 
set(BF_LINK_FLAGS "-Wl,--as-needed")

# Only link in what is absolutely necessary
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${BF_LINK_FLAGS}")

# 
# Compiler specific flags
# 
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -Wall")


# Define the Linux data dir if not defined in a packaging build script already
if(NOT CMAKE_DATA_PATH)
	set(CMAKE_DATA_PATH "${CMAKE_INSTALL_PREFIX}/share")
endif()

message(STATUS "CMAKE_DATA_PATH: ${CMAKE_DATA_PATH}.  Change this by invoking cmake with -DCMAKE_DATA_PATH=<SOME_DIRECTORY>")

# The executable will look for game assets in LINUX_DATA_DIR, hardcoded.
# The default here is fine, but we are overridable in case a packager needs to move these after the fact.
if (NOT LINUX_DATA_DIR)
	# Quotes need to be a part of the definition or the compiler won't understand
	add_definitions(-DLINUX_DATA_DIR="${CMAKE_DATA_PATH}")
else()
	# Yes, this is tautological. No, it does not work without it.
	add_definitions(-DLINUX_DATA_DIR="${LINUX_DATA_DIR}")
endif()

if(NOT CMAKE_BIN_PATH)
	set(CMAKE_BIN_PATH "${CMAKE_INSTALL_PREFIX}/bin")
endif()

# Allow setting of other Linux paths for various resources
if(NOT CMAKE_DESKTOP_DATA_PATH)
	set(CMAKE_DESKTOP_DATA_PATH "${CMAKE_INSTALL_PREFIX}/share")
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


function(BF_PLATFORM_SET_EXECUTABLE_NAME)
	# Do nothing!
endfunction()


function(BF_PLATFORM_SET_TARGET_PROPERTIES targetName)
	# Do nothing!
endfunction()


function(BF_PLATFORM_SET_TARGET_OTHER_PROPERTIES targetName)
	# Do nothing
endfunction()


function(BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES targetName)
	# Do nothing!
endfunction()


function(BF_PLATFORM_INSTALL targetName)
	set(LINUX_PKG_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/packaging/linux/")
	
	# Binaries
	install(TARGETS ${targetName} RUNTIME DESTINATION ${CMAKE_BIN_PATH})

	if(INSTALL_NOTIFIER)
		# Modify python script to have the shebang
		install(CODE "execute_process(COMMAND sed -i -e \"1s@^@#!/usr/bin/env python\\\\n\\\\n@\" ${CMAKE_SOURCE_DIR}/notifier/bitfighter_notifier.py)")
		# Modify python script to use proper path to the system-installed icon
		install(CODE "execute_process(COMMAND sed -i -e \"s@redship48.png@bitfighter.png@\" -e \"s@^ICON_BASE =.*@ICON_BASE = \\\"${CMAKE_DESKTOP_DATA_PATH}/pixmaps/\\\"@\" ${CMAKE_SOURCE_DIR}/notifier/bitfighter_notifier.py)")
		install(PROGRAMS ${CMAKE_SOURCE_DIR}/notifier/bitfighter_notifier.py DESTINATION ${CMAKE_BIN_PATH} RENAME bitfighter_notifier)
	endif()
	
	# Install desktop files
	install(FILES ${LINUX_PKG_RESOURCE_DIR}/bitfighter.desktop DESTINATION ${CMAKE_DESKTOP_DATA_PATH}/applications/)
	install(FILES ${LINUX_PKG_RESOURCE_DIR}/bitfighter.png DESTINATION ${CMAKE_DESKTOP_DATA_PATH}/pixmaps/)
	
	# Manpage
	install(FILES ${LINUX_PKG_RESOURCE_DIR}/bitfighter.6 DESTINATION ${CMAKE_DESKTOP_DATA_PATH}/man/man6/)
	
	# Resources
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/ DESTINATION ${CMAKE_DATA_PATH}/bitfighter/)
	
endfunction()


function(BF_PLATFORM_CREATE_PACKAGES targetName)
	# Do nothing!
endfunction()
