add_executable(tnlping
	EXCLUDE_FROM_ALL
	tnlping.cpp
)

add_dependencies(tnlping 
	tnl
)

BF_PLATFORM_SET_EXTRA_LIBS()

target_link_libraries(tnlping
	tnl
	${EXTRA_LIBS}
)

set_target_properties(tnlping
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
)

# tomcrypt is only needed for resolving some includes in TNL's headers.  
# It is not actually linked in
include_directories(
	${TOMCRYPT_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/tnl
)