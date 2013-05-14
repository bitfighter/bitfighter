find_package(Threads)
find_package(MySQL)


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


if(MSVC)
	set(MASTER_EXTRA_LIBS ws2_32 winmm)
else()
	set(MASTER_EXTRA_LIBS dl m)
endif()


set(MASTER_DEPS tnl tomcrypt)

set(MASTER_LIBS tnl tomcrypt ${CMAKE_THREAD_LIBS_INIT} ${MASTER_EXTRA_LIBS})

set(MASTER_INCLUDES 
	${CMAKE_SOURCE_DIR}/tnl
	${CMAKE_SOURCE_DIR}/libtomcrypt
	${CMAKE_SOURCE_DIR}/boost
)


# Handle different compile of master if MySQL is found or minimal build is requested
# Force
if(MYSQL_FOUND AND NOT MASTER_MINIMAL)
	set(MASTER_DEPS ${MASTER_DEPS} mysql++)
	set(MASTER_LIBS ${MASTER_LIBS} mysql++ ${MYSQL_LIBRARIES})
	set(MASTER_INCLUDES ${MASTER_INCLUDES} ../mysql++ ${MYSQL_INCLUDE_DIR})
else()
	if(MASTER_MINIMAL)
		message(WARNING "MASTER_MINIMAL specified.  'master' will only be built with SQLite")
	else()
		message(WARNING "MySQL was NOT found.  'master' will only be built with SQLite")
	endif()

	set(MASTER_PHPBB_SOURCES "")
endif()


add_executable(master
	EXCLUDE_FROM_ALL
	${MASTER_SOURCES}
	${MASTER_PHPBB_SOURCES}
	${MASTER_HEADERS}
)

add_dependencies(master ${MASTER_DEPS})

target_link_libraries(master ${MASTER_LIBS})

set_target_properties(master
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
)

include_directories(${MASTER_INCLUDES})

# Set extra compile definitions needed for the MySQL build
if(MYSQL_FOUND AND NOT MASTER_MINIMAL)
	set_target_properties(master
		PROPERTIES
		COMPILE_DEFINITIONS "BF_WRITE_TO_MYSQL;VERIFY_PHPBB3"
	)
endif()
