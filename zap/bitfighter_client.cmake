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

add_dependencies(bitfighter_client
	${ALURE_LIB}
	${LUA_LIB}
	tnl
	tomcrypt
	clipper
	poly2tri
)


get_property(CLIENT_DEFS TARGET bitfighter_client PROPERTY COMPILE_DEFINITIONS)

if(USE_GLES)
	set_target_properties(bitfighter_client PROPERTIES COMPILE_DEFINITIONS 
		"${CLIENT_DEFS};BF_USE_GLES"
	)
endif()

set_target_properties(bitfighter_client PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")
