#------------------------------------------------------------------------------
# Copyright Chris Eykamp
# See LICENSE.txt for full copyright information
#------------------------------------------------------------------------------

set(MASTER_SOURCES
	database.cpp
	GameJoltConnector.cpp
	master.cpp
	masterInterface.cpp
	MasterServerConnection.cpp
)

# Extra classes needed for the main master executable
set(MASTER_EXTRA_SOURCES
	../zap/ChatCheck.cpp
	../zap/Color.cpp
	../zap/gameStats.cpp
	../zap/HttpsRequest.cpp
	../zap/IniFile.cpp
	../zap/LevelDatabase.cpp
	../zap/md5wrapper.cpp
	../zap/Settings.cpp
	../zap/stringUtils.cpp
	../zap/Timer.cpp
	../zap/WeaponInfo.cpp
)

file(GLOB MASTER_HEADERS "*.h")


if(MSVC)
	set(MASTER_EXTRA_LIBS ws2_32 winmm)
elseif(APPLE)
	set(MASTER_EXTRA_LIBS "-framework Cocoa" "-framework Foundation")
	set(MASTER_EXTRA_SOURCES ${MASTER_EXTRA_SOURCES} ../zap/Directory.mm)
else()
	set(MASTER_EXTRA_LIBS dl m)
endif()


set(MASTER_DEPS tnl)
# Add tomcrypt if not already found on system
if(NOT TOMCRYPT_FOUND)
	list(APPEND MASTER_DEPS tomcrypt)
endif()

set(MASTER_LIBS 
	tnl
	tomcrypt 
	${SQLITE3_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT} 
	${MASTER_EXTRA_LIBS}
)

set(MASTER_INCLUDES 
	${TOMCRYPT_INCLUDE_DIR}
	${BOOST_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/tnl
	${SQLITE3_INCLUDE_DIR}
)


#
# master_lib OBJECT build
#
add_library(master_lib OBJECT
	EXCLUDE_FROM_ALL
	${MASTER_SOURCES}
	${MASTER_HEADERS}
)

add_dependencies(master_lib
	${MASTER_DEPS}
)

#
# master executable
#
add_executable(master
	EXCLUDE_FROM_ALL
	$<TARGET_OBJECTS:master_lib>
	${MASTER_EXTRA_SOURCES}
	main.cpp
)

add_dependencies(master master_lib)

target_link_libraries(master ${MASTER_LIBS})

set_target_properties(master PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)


include_directories(${MASTER_INCLUDES})

# BF_MASTER workaround to prevent WeaponInfo.cpp from including BfObject
set_target_properties(master_lib master PROPERTIES COMPILE_DEFINITIONS "BF_MASTER")

set_target_properties(master_lib master PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")
