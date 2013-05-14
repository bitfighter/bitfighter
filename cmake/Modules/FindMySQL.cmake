# - Find mysqlclient
# Find the native MySQL includes and library
#
#  MYSQL_INCLUDE_DIR - where to find mysql.h, etc.
#  MYSQL_LIBRARIES   - List of libraries when using MySQL.
#  MYSQL_FOUND       - True if MySQL found.

set(MYSQL_SEARCH_PATHS
	${MYSQL_SEARCH_PATHS}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw        # Fink
	/opt/local # DarwinPorts
	/opt/csw   # Blastwave
	/opt
)

find_path(MYSQL_INCLUDE_DIR 
	NAMES mysql.h
	HINTS ENV MYSQLDIR
	PATH_SUFFIXES include include/mysql
	PATHS ${MYSQL_SEARCH_PATHS}
)

find_library(MYSQL_LIBRARY
	NAMES libmysqlclient mysqlclient libmysqlclient_r mysqlclient_r
	HINTS ENV MYSQLDIR
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
	PATHS ${MYSQL_SEARCH_PATHS}
)


if(MYSQL_LIBRARY)
	set(MYSQL_LIBRARIES ${MYSQL_LIBRARY})
else()
	set(MYSQL_LIBRARIES)
endif()


# Handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MYSQL DEFAULT_MSG MYSQL_LIBRARIES MYSQL_INCLUDE_DIR)
