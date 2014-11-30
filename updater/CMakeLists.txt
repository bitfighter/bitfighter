# This is the CMakeLists.txt for the bitfighter Windows updater, based on GUP
if(WIN32)


add_executable(bfup
	WIN32                           # make it a GUI instead of console app
	EXCLUDE_FROM_ALL
	src/TinyXml/tinystr.cpp
	src/TinyXml/tinystr.h
	src/TinyXml/tinyxml.cpp
	src/TinyXml/tinyxml.h
	src/TinyXml/tinyxmlerror.cpp
	src/TinyXml/tinyxmlparser.cpp
	src/xmlTools.cpp
	src/xmlTools.h
	src/gup.rc
	src/winmain.cpp
)

# Use correct library if Win64 or Win32
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(CURL_LIB_DIR ${CMAKE_SOURCE_DIR}/updater/lib/win64/)
else()
	set(CURL_LIB_DIR ${CMAKE_SOURCE_DIR}/updater/lib/)
endif()


set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/updater/src/libcurl/include")
set(CURL_LIBRARY    "${CURL_LIB_DIR}libcurl.lib")

find_package(CURL REQUIRED)

target_link_libraries(bfup comctl32 shlwapi ${CURL_LIBRARIES})

# Set up output dir
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe/updater/ outputDir)

set_target_properties(bfup PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${outputDir})

if(MSVC)
	# Work around the "Debug", "Release", etc. directories Visual Studio tries to add
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
		set_target_properties(bfup PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${outputDir})
	endforeach()
endif()

include_directories(
	${CURL_INCLUDE_DIR} 
	src/TinyXml
)


# Set up post-build resources
file(TO_NATIVE_PATH ${CURL_LIB_DIR} curlLibDir)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/updater/resource/ resDir)
if(MSYS OR CYGWIN OR XCOMPILE)
	set(LIBCURL_COPY_CMD cp ${curlLibDir}libcurl.dll ${outputDir})
	set(RES_COPY_CMD cp ${resDir}* ${outputDir})
else()
	set(LIBCURL_COPY_CMD xcopy /d /y ${curlLibDir}libcurl.dll ${outputDir})
	set(RES_COPY_CMD xcopy /d /y ${resDir}* ${outputDir})
endif()

# Copy resources
add_custom_command(TARGET bfup POST_BUILD 
	COMMAND ${LIBCURL_COPY_CMD}
	COMMAND ${RES_COPY_CMD}
)


endif(WIN32)