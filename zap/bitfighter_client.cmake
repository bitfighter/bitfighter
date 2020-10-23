#
# Client OBJECT build - includes resources only compiled into the client
# 

BF_PLATFORM_SET_EXTRA_SOURCES()

add_library(bitfighter_client OBJECT
	${SHARED_SOURCES}
	${CLIENT_SOURCES}
	${BITFIGHTER_HEADERS}
	${OTHER_HEADERS}
)


# If certain system libs were not found, add the in-tree variants as dependencies
set(CLIENT_EXTRA_DEPS "")
if(NOT ALURE_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS alure)
endif()
if(NOT TOMCRYPT_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS tomcrypt)
endif()
if(NOT CLIPPER_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS clipper)
endif()
if(NOT POLY2TRI_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS poly2tri)
endif()


add_dependencies(bitfighter_client
	${LUA_LIB}
	tnl
	${CLIENT_EXTRA_DEPS}
)

if(USE_GLES)
	get_property(CLIENT_DEFS TARGET bitfighter_client PROPERTY COMPILE_DEFINITIONS)
	set_target_properties(bitfighter_client
		PROPERTIES
		COMPILE_DEFINITIONS "${CLIENT_DEFS};BF_USE_GLES"
	)
endif()

set_target_properties(bitfighter_client PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")
