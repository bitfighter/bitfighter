# Define the Linux data dir if not defined in a packaging build script already
if(MINGW)
	set(BF_LINK_FLAGS "-Wl,--as-needed -static-libgcc")
else()
	set(BF_LINK_FLAGS "-Wl,--as-needed")
endif()

find_package(VorbisFile)

if(MINGW)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
endif()

set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")

# Only link in what is absolutely necessary
set(CMAKE_EXE_LINKER_FLAGS ${BF_LINK_FLAGS})

if("${CMAKE_SYSTEM}" MATCHES "Linux")
	if(NOT LINUX_DATA_DIR)
		set(LINUX_DATA_DIR "/usr/share")
	endif()

	message(STATUS "LINUX_DATA_DIR: ${LINUX_DATA_DIR}.  Change this by invoking cmake with -DLINUX_DATA_DIR=<SOME_DIRECTORY>")

	# Quotes need to be a part of the definition or the compiler won't understand
	add_definitions(-DLINUX_DATA_DIR="${LINUX_DATA_DIR}")
endif()
set(EXTRA_LIBS dl m)
add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)

function(BF_PLATFORM_INSTALL)
	install(TARGETS bitfighter RUNTIME DESTINATION bin)
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/
		DESTINATION share/games/bitfighter/
	)
endfunction()
