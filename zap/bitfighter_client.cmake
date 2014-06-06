#
# Full client build
# 

add_library(bitfighter_client
	${SHARED_SOURCES}
	${CLIENT_SOURCES}
	${BITFIGHTER_HEADERS}
	${OTHER_HEADERS}
)

add_dependencies(bitfighter_client
	alure
	${LUA_LIB}
	tnl
	tomcrypt
)

target_link_libraries(bitfighter_client
	${CLIENT_LIBS}
	${SHARED_LIBS}
)


# Where to put the executable
set_target_properties(bitfighter_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

if(USE_GLES)
	get_property(CLIENT_DEFS TARGET bitfighter_client PROPERTY COMPILE_DEFINITIONS)
	set_target_properties(bitfighter_client
		PROPERTIES
		COMPILE_DEFINITIONS "${CLIENT_DEFS};BF_USE_GLES"
	)
endif()

set_target_properties(bitfighter_client PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")


BF_PLATFORM_SET_TARGET_PROPERTIES(bitfighter_client)
