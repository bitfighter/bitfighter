#
# Full client build
# 

add_executable(bitfighter
	${SHARED_SOURCES}
	${CLIENT_SOURCES}
	${BITFIGHTER_HEADERS}
	${OTHER_HEADERS}
	main.cpp
)

add_dependencies(bitfighter
	alure
	${LUA_LIB}
	tnl
	tomcrypt
)

target_link_libraries(bitfighter
	${CLIENT_LIBS}
	${SHARED_LIBS}
)


# Where to put the executable
set_target_properties(bitfighter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

if(USE_GLES)
	get_property(CLIENT_DEFS TARGET bitfighter PROPERTY COMPILE_DEFINITIONS)
	set_target_properties(bitfighter
		PROPERTIES
		COMPILE_DEFINITIONS "${CLIENT_DEFS};BF_USE_GLES"
	)
endif()

set_target_properties(bitfighter PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")


BF_PLATFORM_SET_TARGET_PROPERTIES(bitfighter)

BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES(bitfighter)

BF_PLATFORM_INSTALL(bitfighter)

BF_PLATFORM_CREATE_PACKAGES(bitfighter)
