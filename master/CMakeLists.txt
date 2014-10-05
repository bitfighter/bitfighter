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
	../zap/HttpRequest.cpp
	../zap/IniFile.cpp
	../zap/LevelDatabase.cpp
	../zap/md5wrapper.cpp
	../zap/Settings.cpp
	../zap/stringUtils.cpp
	../zap/Timer.cpp
	../zap/WeaponInfo.cpp
)

set(MASTER_PHPBB_SOURCES
	authenticator.cpp
	phpbbhash.cpp
)

file(GLOB MASTER_HEADERS "*.h")


if(MSVC)
	set(MASTER_EXTRA_LIBS ws2_32 winmm)
else()
	set(MASTER_EXTRA_LIBS dl m)
endif()


set(MASTER_DEPS tnl tomcrypt sqlite)

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


# Handle different compile of master if MySQL is found or minimal build is requested
# Force
if(MYSQL_FOUND AND NOT MASTER_MINIMAL)
	set(MASTER_DEPS ${MASTER_DEPS} mysql++)
	set(MASTER_LIBS ${MASTER_LIBS} mysql++ ${MYSQL_LIBRARIES})
	set(MASTER_INCLUDES ${MASTER_INCLUDES} ../mysql++ ${MYSQL_INCLUDE_DIR})
else()
	if(MASTER_MINIMAL)
		message(STATUS "MASTER_MINIMAL specified.  'master' will only be built with SQLite")
	else()
		message(STATUS "MySQL was NOT found.  'master' will only be built with SQLite")
	endif()

	set(MASTER_PHPBB_SOURCES "")
endif()


#
# master_lib OBJECT build
#
add_library(master_lib OBJECT
	EXCLUDE_FROM_ALL
	${MASTER_SOURCES}
	${MASTER_PHPBB_SOURCES}
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

# Set extra compile definitions needed for the MySQL build
if(MYSQL_FOUND AND NOT MASTER_MINIMAL)
	set_target_properties(master_lib master PROPERTIES COMPILE_DEFINITIONS "BF_WRITE_TO_MYSQL;VERIFY_PHPBB3;BF_MASTER")
else()
	# BF_MASTER workaround to prevent WeaponInfo.cpp from including BfObject
	set_target_properties(master_lib master PROPERTIES COMPILE_DEFINITIONS "BF_MASTER")
endif()

set_target_properties(master_lib master PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")
