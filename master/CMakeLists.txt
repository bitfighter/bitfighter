find_package(Threads)
find_package(MySQL)

# TODO: master server feature configuration

set(MASTER_SOURCES
	master.cpp
	masterInterface.cpp
	config.cpp
	database.cpp
	../zap/ChatCheck.cpp
	../zap/Color.cpp
	../zap/gameStats.cpp
	../zap/IniFile.cpp
	../zap/stringUtils.cpp
	../zap/WeaponInfo.cpp
	../sqlite/sqlite3.c
)

set(MASTER_PHPBB_SOURCES
	authenticator.cpp
	phpbbhash.cpp
)

file(GLOB MASTER_HEADERS "*.h")

add_executable(master
	EXCLUDE_FROM_ALL
	${MASTER_SOURCES}
	${MASTER_HEADERS}
)

add_dependencies(master
	tnl tomcrypt
)

if(MSVC)
	set(MASTER_EXTRA_LIBS ws2_32 winmm)
else()
	set(MASTER_EXTRA_LIBS dl m)
endif()

target_link_libraries(master
	tnl tomcrypt ${CMAKE_THREAD_LIBS_INIT} ${MASTER_EXTRA_LIBS}
)

set_target_properties(master
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
)


include_directories(
	${CMAKE_SOURCE_DIR}/tnl
	${CMAKE_SOURCE_DIR}/libtomcrypt
	${CMAKE_SOURCE_DIR}/boost
)

# If MySQL was not found, then master will be compiled with just SQLite
if(FOUND_MYSQL)
	add_executable(master
		EXCLUDE_FROM_ALL
		${MASTER_SOURCES}
		${MASTER_PHPBB_SOURCES}
	)

	add_dependencies(master mysql++)

	target_link_libraries(master
		mysql++ ${MYSQL_LIBRARIES}
	)

	set_target_properties(master
		PROPERTIES
		COMPILE_DEFINITIONS "BF_WRITE_TO_MYSQL;VERIFY_PHPBB3"
	)

	include_directories(../mysql++ ${MYSQL_INCLUDE_DIR})
else()
	message(WARNING "MySQL was NOT found.  'master' will only be built with SQLite")
endif()
