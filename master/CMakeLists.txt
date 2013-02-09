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

if(FOUND_MYSQL)
	add_executable(master
		EXCLUDE_FROM_ALL
		${MASTER_SOURCES}
		${MASTER_PHPBB_SOURCES}
	)

	add_dependencies(master
		tnl tomcrypt mysql++
	)

	target_link_libraries(master
		tnl tomcrypt mysql++ ${MYSQL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} dl m
	)

	set_target_properties(master
		PROPERTIES
		COMPILE_DEFINITIONS BF_WRITE_TO_MYSQL
		COMPILE_DEFINITIONS VERIFY_PHPBB3
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
	)

	include_directories(../tnl ../mysql++ ../libtomcrypt ${MYSQL_INCLUDE_DIR})
endif()
